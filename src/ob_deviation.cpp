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


LRESULT CALLBACK DeviationDlgHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool init;
	DEVIATIONOBJ * st;
	
	st = (DEVIATIONOBJ *) actobject;
    if ((st==NULL)||(st->type!=OB_DEVIATION)) return(FALSE);	

	switch( message )
	{
		case WM_INITDIALOG:
		{
				SCROLLINFO lpsi;
			    lpsi.cbSize=sizeof(SCROLLINFO);
				lpsi.fMask=SIF_RANGE|SIF_POS;
				// ENDRING: Sett sliderens rekkevidde til sekunder, f.eks. 1 til 300 (5 minutter)
				lpsi.nMin=1; lpsi.nMax= 300;
				SetScrollInfo(GetDlgItem(hDlg,IDC_DEVIATIONINTERVALBAR),SB_CTL,&lpsi, TRUE);
				
				init = true;

				// ENDRING: Bruk interval_seconds for å sette slider og tekstboks
				SetScrollPos(GetDlgItem(hDlg,IDC_DEVIATIONINTERVALBAR), SB_CTL,st->interval_seconds, TRUE);
				SetDlgItemInt(hDlg, IDC_DEVIATIONINTERVAL, st->interval_seconds, FALSE);
                
				init = false;
				break;		
		}
		case WM_CLOSE:
			    EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			break;
		case WM_COMMAND:
			return TRUE;
			break;
		case WM_HSCROLL:
		{
			int nNewPos; 
			if (!init && (nNewPos = get_scrollpos(wParam,lParam)) >= 0)
			{   
				if (lParam == (long) GetDlgItem(hDlg,IDC_DEVIATIONINTERVALBAR))  
				{
					// ENDRING: nNewPos er nå i sekunder. Oppdater tekstboks og kall den nye funksjonen.
					SetDlgItemInt(hDlg, IDC_DEVIATIONINTERVAL, nNewPos, TRUE);
                    st->change_interval_seconds(nNewPos);
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
		squares[i] = 0.0; // ENDRING: Bør også initialisere squares-arrayen
    }

    // ENDRING: Sett standard til 60 sekunder og kalkuler antall samples
    interval_seconds = 60;
	// Sørg for at TTY.samplingrate har en fornuftig verdi ved oppstart
	if (TTY.samplingrate > 0) {
		interval = interval_seconds * TTY.samplingrate;
	} else {
		interval = interval_seconds * 256; // Fallback til 256 Hz hvis TTY.samplingrate er 0
	}
	
    writepos = 0;
    added = 0;
}
	
void DEVIATIONOBJ::make_dialog(void)
{
	display_toolbox(hDlg=CreateDialog(hInst, (LPCTSTR)IDD_DEVIATIONBOX, ghWndStatusbox, (DLGPROC)DeviationDlgHandler));
}

void DEVIATIONOBJ::load(HANDLE hFile) 
{
   load_object_basics(this);
   // ENDRING: Last inn interval_seconds i stedet for interval
   load_property("interval_seconds",P_INT,&interval_seconds);
   // ENDRING: Rekalkuler interval i samples etter lasting
   if (TTY.samplingrate > 0) {
	   interval = interval_seconds * TTY.samplingrate;
   } else {
	   interval = interval_seconds * 256; // Fallback
   }
}

void DEVIATIONOBJ::save(HANDLE hFile) 
{
	save_object_basics(hFile,this);
	// ENDRING: Lagre interval_seconds i stedet for interval
    save_property(hFile,"interval_seconds",P_INT,&interval_seconds);
}
	
void DEVIATIONOBJ::incoming_data(int port, float value)
{
	// Denne funksjonen trenger ingen endringer, da den bruker 'interval' (i samples)
	// som nå blir korrekt satt av change_interval_seconds()
	if (value!=INVALID_VALUE)
	{
		samples[writepos] = value;
   		meanaccu += value;
		added++;
		if (added > interval)
		{
		    int oldest = writepos - interval;
			if (oldest < 0)
	    		oldest += DEVIATION_NUMSAMPLES;
		    meanaccu -= samples[oldest];
			devaccu -=  squares[oldest];
			added = interval;
		}
		
		// Unngå deling på null hvis 'added' skulle være 0
		if (added > 0) {
			mean = meanaccu / added;
		} else {
			mean = 0.0f;
		}
		
		squares[writepos]=(value-mean)*(value-mean);
		devaccu += squares[writepos];

		if (added > 0) {
			deviation = (float) sqrt ((double) (devaccu / added));
		} else {
			deviation = 0.0f;
		}

		writepos++;
		if (writepos >= DEVIATION_NUMSAMPLES)
    		writepos = 0;
	}
}
	
void DEVIATIONOBJ::work(void)
{	
	pass_values(0, deviation);
	pass_values(1, mean);
	
}

// ENDRING: Implementerer den nye funksjonen
void DEVIATIONOBJ::change_interval_seconds(int newinterval_seconds)
{
	interval_seconds = newinterval_seconds;

	// Beregn nytt intervall i samples basert på global TTY.samplingrate
	if (TTY.samplingrate > 0) {
		interval = interval_seconds * TTY.samplingrate;
	} else {
		interval = interval_seconds * 256; // Bruk en fornuftig standardverdi hvis TTY.samplingrate ikke er satt
	}

	// Sikkerhetssjekk for å unngå buffer overflow
	if (interval >= DEVIATION_NUMSAMPLES) {
		interval = DEVIATION_NUMSAMPLES - 1;
	}
	
	// Reset kalkulasjonen
	added = 0;
	meanaccu = 0;
	devaccu=0;
}

DEVIATIONOBJ::~DEVIATIONOBJ() {}