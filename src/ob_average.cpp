/* -----------------------------------------------------------------------------

   BrainBay  -  OpenSource Biofeedback Software

  MODULE:  OB_AVERAGE.CPP
  Author:  Chris Veigl


  This Object outputs the Average of n Samples captured from it's input-port

 This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; See the
  GNU General Public License for more details.

  
-------------------------------------------------------------------------------------*/

#include "brainBay.h"
#include "ob_average.h"
#include <iostream>
#include <tchar.h>

using namespace std;

// Prototyp for hjelpefunksjon vi lager lenger nede i filen.
void UpdateAverageDialogUI(HWND hDlg, AVERAGEOBJ* st);

AVERAGEOBJ::AVERAGEOBJ(int num) : BASE_CL()
{
	outports = 1;
	inports = 1;
	width=75;
    strcpy(in_ports[0].in_name,"in");
	strcpy(out_ports[0].out_name,"out");
    accumulator = 0.0;
    for (int i = 0; i < AVERAGE_NUMSAMPLES; i++)
    {
    	samples[i] = 0.0;
    }
    
    // ENDRING: Sett standardmodus og verdier.
    mode = MODE_SECONDS; // Standard til tidsbasert.
    interval_setting = 5;
    update_settings(interval_setting, mode); // Kall den nye funksjonen for å kalkulere 'interval'.

    writepos = 0;
    added = 0;
}
	
void AVERAGEOBJ::session_start(void)
{
  //	added = 0;
  //	accumulator = 0;
}
void AVERAGEOBJ::session_reset(void)
{
	added = 0;
	accumulator = 0;
}

void AVERAGEOBJ::session_pos(long pos)
{
	added = 0;
	accumulator = 0;
}

void AVERAGEOBJ::make_dialog(void)
{
	display_toolbox(hDlg=CreateDialog(hInst, (LPCTSTR)IDD_AVERAGEBOX, ghWndStatusbox, (DLGPROC)AverageDlgHandler));
}

void AVERAGEOBJ::load(HANDLE hFile) 
{
   load_object_basics(this);
   // ENDRING: Last inn både modus og innstilling.
   load_property("mode", P_INT, &mode);
   load_property("interval_setting", P_INT, &interval_setting);
   update_settings(interval_setting, mode); // Rekalkuler 'interval' etter lasting.
}

void AVERAGEOBJ::save(HANDLE hFile) 
{
	save_object_basics(hFile,this);
    // ENDRING: Lagre både modus og innstilling.
    save_property(hFile,"mode", P_INT, &mode);
    save_property(hFile,"interval_setting", P_INT, &interval_setting);
}
	
void AVERAGEOBJ::incoming_data(int port, float value)
{
	if (value!=INVALID_VALUE)
	{
   		accumulator += value;
		added++;

		if (interval > 0) // Sjekk for > 0 for å unngå feil
		{
			samples[writepos] = value;
			if (added > interval)
			{
				int oldest = writepos - interval;
				if (oldest < 0)
	    			oldest += AVERAGE_NUMSAMPLES;
			    accumulator -= samples[oldest];
				added = interval;
			}
			writepos++;
			if (writepos >= AVERAGE_NUMSAMPLES)
    			writepos = 0;
		}
	}
}
	
void AVERAGEOBJ::work(void)
{
    float average;
	if (added)
	{  
		average = accumulator / added;
		pass_values(0, average);
	}
}

// ENDRING: Implementerer den nye, sentrale logikk-funksjonen.
void AVERAGEOBJ::update_settings(int new_value, int new_mode)
{
    mode = new_mode;
    interval_setting = new_value;

    if (mode == MODE_SECONDS) {
        // Kalkuler intervall i samples.
        if (TTY.samplingrate > 0) {
            interval = interval_setting * TTY.samplingrate;
        } else {
            interval = interval_setting * 256; // Fallback
        }
    } else { // mode == MODE_EVENTS
        // Intervallet er bare antall hendelser.
        interval = interval_setting;
    }

    // Sikkerhetssjekk.
    if (interval >= AVERAGE_NUMSAMPLES) {
        interval = AVERAGE_NUMSAMPLES - 1;
    }
    if (interval < 1) { // Sørg for at intervallet er minst 1
        interval = 1;
    }

	added = 0;
	accumulator = 0;
}

AVERAGEOBJ::~AVERAGEOBJ() {}

// ENDRING: Dialog-handleren er kraftig modifisert.
LRESULT CALLBACK AverageDlgHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool init;
	AVERAGEOBJ * st;
	
	st = (AVERAGEOBJ *) actobject;
    if ((st==NULL)||(st->type!=OB_AVERAGE)) return(FALSE);	

	switch( message )
	{
		case WM_INITDIALOG:
				init = true;
                // Sett radioknapp basert på lagret modus.
				CheckRadioButton(hDlg, IDC_RADIO_SECONDS, IDC_RADIO_EVENTS, (st->mode == MODE_SECONDS) ? IDC_RADIO_SECONDS : IDC_RADIO_EVENTS);
                // Oppdater resten av UI-et (slider, tekst, etc.).
                UpdateAverageDialogUI(hDlg, st);
				init = false;
				break;		
		case WM_CLOSE:
			    EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			break;
		case WM_COMMAND:
            // Håndter klikk på radioknappene.
            switch(LOWORD(wParam))
            {
                case IDC_RADIO_SECONDS:
                    st->update_settings(st->interval_setting, MODE_SECONDS);
                    UpdateAverageDialogUI(hDlg, st);
                    break;
                case IDC_RADIO_EVENTS:
                    st->update_settings(st->interval_setting, MODE_EVENTS);
                    UpdateAverageDialogUI(hDlg, st);
                    break;
            }
			return TRUE;
			break;
		case WM_HSCROLL:
		{
			int nNewPos; 
			if (!init && (nNewPos = get_scrollpos(wParam,lParam)) >= 0)
			{   
				if (lParam == (long) GetDlgItem(hDlg,IDC_AVERAGEINTERVALBAR))  
				{
					SetDlgItemInt(hDlg, IDC_AVERAGEINTERVAL, nNewPos, TRUE);
                    // Oppdater innstillingene med den nye verdien fra slideren.
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

// ENDRING: Ny hjelpefunksjon for å oppdatere UI-elementene.
void UpdateAverageDialogUI(HWND hDlg, AVERAGEOBJ* st)
{
    SCROLLINFO lpsi;
    lpsi.cbSize = sizeof(SCROLLINFO);
    lpsi.fMask = SIF_RANGE | SIF_POS;

    if (st->mode == MODE_SECONDS) {
        // Denne vil nå fungere fordi _T er definert
        SetDlgItemText(hDlg, IDC_INTERVAL_UNITS_LABEL, _T("seconds"));
        lpsi.nMin = 1;
        lpsi.nMax = 300;
    } else { // mode == MODE_EVENTS
        // Denne vil nå fungere fordi _T er definert
        SetDlgItemText(hDlg, IDC_INTERVAL_UNITS_LABEL, _T("events"));
        lpsi.nMin = 1;
        lpsi.nMax = 1000; 
    }
    
    lpsi.nPos = st->interval_setting;
    SetScrollInfo(GetDlgItem(hDlg, IDC_AVERAGEINTERVALBAR), SB_CTL, &lpsi, TRUE);
    SetDlgItemInt(hDlg, IDC_AVERAGEINTERVAL, st->interval_setting, FALSE);
}