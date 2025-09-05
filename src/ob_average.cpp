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

using namespace std;

AVERAGEOBJ::AVERAGEOBJ(int num) : BASE_CL()
{
	outports = 1;
	inports = 1;
	width=75;
    strcpy(in_ports[0].in_name,"in");
	strcpy(out_ports[0].out_name,"out");
    accumulator = 0.0;
    // ENDRING: Bruker den nye makroen.
    for (int i = 0; i < AVERAGE_NUMSAMPLES; i++)
    {
    	samples[i] = 0.0;
    }
    
    // ENDRING: Sett standard til 5 sekunder og kalkuler antall samples.
    interval_seconds = 5;
    if (TTY.samplingrate > 0) {
        interval = interval_seconds * TTY.samplingrate;
    } else {
        interval = interval_seconds * 256; // Fallback
    }

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
   // ENDRING: Last inn interval_seconds i stedet for interval.
   load_property("interval_seconds",P_INT,&interval_seconds);
   // ENDRING: Rekalkuler interval i samples etter lasting.
   if (TTY.samplingrate > 0) {
	   interval = interval_seconds * TTY.samplingrate;
   } else {
	   interval = interval_seconds * 256; // Fallback
   }
}

void AVERAGEOBJ::save(HANDLE hFile) 
{
	save_object_basics(hFile,this);
    // ENDRING: Lagre interval_seconds i stedet for interval.
    save_property(hFile,"interval_seconds",P_INT,&interval_seconds);
}
	
void AVERAGEOBJ::incoming_data(int port, float value)
{
	if (value!=INVALID_VALUE)
	{
   		accumulator += value;
		added++;

		if (interval) 
		{
			samples[writepos] = value;
			if (added > interval)
			{
				int oldest = writepos - interval;
				if (oldest < 0)
                    // ENDRING: Bruker den nye makroen.
	    			oldest += AVERAGE_NUMSAMPLES;
			    accumulator -= samples[oldest];
				added = interval;
			}
			writepos++;
            // ENDRING: Bruker den nye makroen.
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

// ENDRING: Implementerer den nye funksjonen.
void AVERAGEOBJ::change_interval_seconds(int newinterval_seconds)
{
	interval_seconds = newinterval_seconds;

    // Beregn nytt intervall i samples basert på global samplingrate.
    if (TTY.samplingrate > 0) {
        interval = interval_seconds * TTY.samplingrate;
    } else {
        interval = interval_seconds * 256; // Fallback
    }

    // Sikkerhetssjekk for å unngå buffer overflow.
    if (interval >= AVERAGE_NUMSAMPLES) {
        interval = AVERAGE_NUMSAMPLES - 1;
    }

	added = 0;
	accumulator = 0;
}

AVERAGEOBJ::~AVERAGEOBJ() {}

LRESULT CALLBACK AverageDlgHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool init;
	AVERAGEOBJ * st;
	
	st = (AVERAGEOBJ *) actobject;
    if ((st==NULL)||(st->type!=OB_AVERAGE)) return(FALSE);	

	switch( message )
	{
		case WM_INITDIALOG:
		{
				SCROLLINFO lpsi;
			    lpsi.cbSize=sizeof(SCROLLINFO);
				lpsi.fMask=SIF_RANGE|SIF_POS;
                // ENDRING: Sett sliderens rekkevidde til sekunder, f.eks. 1 til 300.
				lpsi.nMin=1; lpsi.nMax=300;
				SetScrollInfo(GetDlgItem(hDlg,IDC_AVERAGEINTERVALBAR),SB_CTL,&lpsi, TRUE);
				
				init = true;

                // ENDRING: Bruk interval_seconds for å sette slider og tekstboks.
				SetScrollPos(GetDlgItem(hDlg,IDC_AVERAGEINTERVALBAR), SB_CTL,st->interval_seconds, TRUE);
				SetDlgItemInt(hDlg, IDC_AVERAGEINTERVAL, st->interval_seconds, FALSE);
                
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
				if (lParam == (long) GetDlgItem(hDlg,IDC_AVERAGEINTERVALBAR))  
				{
                    // ENDRING: nNewPos er nå i sekunder. Oppdater tekstboks og kall den nye funksjonen.
					SetDlgItemInt(hDlg, IDC_AVERAGEINTERVAL, nNewPos, TRUE);
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