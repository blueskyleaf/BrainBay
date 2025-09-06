/* -----------------------------------------------------------------------------

   BrainBay  -  OpenSource Biofeedback Software

  MODULE:  OB_DEVIATION.CPP
  Author:  Chris Veigl


  This Object outputs the Standard Deviation and Mean of n 
  Samples captured from it's input-port

 This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; See the
  GNU General Public License for more details.

  
-------------------------------------------------------------------------------------*/

#include "brainBay.h"
#include "ob_deviation.h"
#include <tchar.h> // Inkluder for _T() makroen

// Prototyp for hjelpefunksjon
void UpdateDeviationDialogUI(HWND hDlg, DEVIATIONOBJ* st);

// ENDRING: Dialog-handleren er kraftig modifisert.
LRESULT CALLBACK DeviationDlgHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool init;
	DEVIATIONOBJ * st;
	
	st = (DEVIATIONOBJ *) actobject;
    if ((st==NULL)||(st->type!=OB_DEVIATION)) return(FALSE);	

	switch( message )
	{
		case WM_INITDIALOG:
				init = true;
				CheckRadioButton(hDlg, IDC_RADIO_SECONDS_DEV, IDC_RADIO_EVENTS_DEV, (st->mode == MODE_SECONDS_DEV) ? IDC_RADIO_SECONDS_DEV : IDC_RADIO_EVENTS_DEV);
                UpdateDeviationDialogUI(hDlg, st);
				init = false;
				break;		
		case WM_CLOSE:
			    EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			break;
		case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_RADIO_SECONDS_DEV:
                    st->update_settings(st->interval_setting, MODE_SECONDS_DEV);
                    UpdateDeviationDialogUI(hDlg, st);
                    break;
                case IDC_RADIO_EVENTS_DEV:
                    st->update_settings(st->interval_setting, MODE_EVENTS_DEV);
                    UpdateDeviationDialogUI(hDlg, st);
                    break;
            }
			return TRUE;
			break;
		case WM_HSCROLL:
		{
			int nNewPos; 
			if (!init && (nNewPos = get_scrollpos(wParam,lParam)) >= 0)
			{   
				if (lParam == (long) GetDlgItem(hDlg,IDC_DEVIATIONINTERVALBAR))  
				{
					SetDlgItemInt(hDlg, IDC_DEVIATIONINTERVAL, nNewPos, TRUE);
                    st->update_settings(nNewPos, st->mode);
				}
			}
			break;
		}
		case WM_SIZE:
		case WM_MOVE:  update_toolbox_position(hDlg);
		break;
		return(TRUE);
	}
	return FALSE;
}

DEVIATIONOBJ::DEVIATIONOBJ(int num) : BASE_CL()
{
	outports = 2;
	inports = 1;
	width=75;
    strcpy(in_ports[0].in_name,"in");
	strcpy(out_ports[0].out_name,"dev");
	strcpy(out_ports[1].out_name,"mean");
    meanaccu = 0.0; devaccu=0.0;
    for (int i = 0; i < DEVIATION_NUMSAMPLES; i++)
    {
    	samples[i] = 0.0;
		squares[i] = 0.0;
    }

    // ENDRING: Sett standardmodus og verdier.
    mode = MODE_SECONDS_DEV;
    interval_setting = 60; // God standard for HRV
    update_settings(interval_setting, mode);

    writepos = 0;
    added = 0;
    last_value = INVALID_VALUE;
}
	
void DEVIATIONOBJ::make_dialog(void)
{
	display_toolbox(hDlg=CreateDialog(hInst, (LPCTSTR)IDD_DEVIATIONBOX, ghWndStatusbox, (DLGPROC)DeviationDlgHandler));
}

void DEVIATIONOBJ::load(HANDLE hFile) 
{
   load_object_basics(this);
   // ENDRING: Last inn både modus og innstilling.
   load_property("mode", P_INT, &mode);
   load_property("interval_setting", P_INT, &interval_setting);
   update_settings(interval_setting, mode);
}

void DEVIATIONOBJ::save(HANDLE hFile) 
{
	save_object_basics(hFile,this);
    // ENDRING: Lagre både modus og innstilling.
    save_property(hFile,"mode", P_INT, &mode);
    save_property(hFile,"interval_setting", P_INT, &interval_setting);
}

	
void DEVIATIONOBJ::incoming_data(int port, float value)
{
	bool new_event = false;

	if (mode == MODE_SECONDS_DEV) {
		if (value != INVALID_VALUE) {
			new_event = true;
		}
	} else { // mode == MODE_EVENTS_DEV
		if (value != INVALID_VALUE && last_value == INVALID_VALUE) {
			new_event = true;
		}
	}
	
	// Kjør kalkulasjonen kun hvis en gyldig hendelse har skjedd.
	if (new_event)
	{
		// Denne logikkblokken er den originale, velprøvde logikken.
		// Den fungerer for begge moduser, så lenge den kun kjøres én gang per hendelse.
		
		// 1. Hvis bufferen er full, fjern de eldste dataene.
		if (added >= interval)
		{
		    int oldest = writepos - interval;
			if (oldest < 0)
	    		oldest += DEVIATION_NUMSAMPLES;
		    meanaccu -= samples[oldest];
			devaccu -= squares[oldest];
		} else {
			added++;
		}
		
		// 2. Legg til den nye verdien.
		samples[writepos] = value;
   		meanaccu += value;
		
		// 3. Rekalkuler gjennomsnitt.
		mean = meanaccu / added;

		// 4. Rekalkuler og legg til den nye kvadratsummen.
		squares[writepos] = (value - mean) * (value - mean);
		devaccu += squares[writepos];

		// 5. Rekalkuler standardavvik.
		deviation = (float) sqrt((double)(devaccu / added));

		// 6. Oppdater skrivepeker.
		writepos++;
		if (writepos >= DEVIATION_NUMSAMPLES)
    		writepos = 0;
	}

	// Husk alltid den siste verdien for neste "edge detection".
	last_value = value;
}
	
void DEVIATIONOBJ::work(void)
{	
	pass_values(0, deviation);
	pass_values(1, mean);
}

void DEVIATIONOBJ::update_settings(int new_value, int new_mode)
{
    mode = new_mode;
    interval_setting = new_value;

    if (mode == MODE_SECONDS_DEV) {
        if (TTY.samplingrate > 0) {
            interval = interval_setting * TTY.samplingrate;
        } else {
            interval = interval_setting * 256;
        }
    } else { // mode == MODE_EVENTS_DEV
        interval = interval_setting;
    }

    if (interval >= DEVIATION_NUMSAMPLES) {
        interval = DEVIATION_NUMSAMPLES - 1;
    }
    if (interval < 1) {
        interval = 1;
    }

	// Full nullstilling av tilstanden.
	added = 0;
    writepos = 0;
	meanaccu = 0;
	devaccu = 0;
    mean = 0;
    deviation = 0;
    last_value = INVALID_VALUE;
}

DEVIATIONOBJ::~DEVIATIONOBJ() {}

void UpdateDeviationDialogUI(HWND hDlg, DEVIATIONOBJ* st)
{
    SCROLLINFO lpsi;
    lpsi.cbSize = sizeof(SCROLLINFO);
    lpsi.fMask = SIF_RANGE | SIF_POS;

    if (st->mode == MODE_SECONDS_DEV) {
        SetDlgItemText(hDlg, IDC_INTERVAL_UNITS_LABEL_DEV, _T("seconds"));
        lpsi.nMin = 1;
        lpsi.nMax = 300;
    } else { // mode == MODE_EVENTS_DEV
        SetDlgItemText(hDlg, IDC_INTERVAL_UNITS_LABEL_DEV, _T("events"));
        lpsi.nMin = 2; // Trenger minst 2 punkter for et meningsfylt standardavvik
        lpsi.nMax = 1000; 
    }
    
    lpsi.nPos = st->interval_setting;
    SetScrollInfo(GetDlgItem(hDlg, IDC_DEVIATIONINTERVALBAR), SB_CTL, &lpsi, TRUE);
    SetDlgItemInt(hDlg, IDC_DEVIATIONINTERVAL, st->interval_setting, FALSE);
}