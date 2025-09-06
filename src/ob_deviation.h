/* -----------------------------------------------------------------------------

    BrainBay  -  OpenSource Biofeedback Software, contact: chris@shifz.org
  
  MODULE: OB_DEVIATION.H:  declarations for the Deviation-Object
  Author: Chris Veigl

  This Object outputs the Standard Deviation and Mean of n 
  Samples captured from it's input-port

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; See the
  GNU General Public License for more details.

-----------------------------------------------------------------------------*/

#include "brainBay.h"

// ENDRING: Økt bufferstørrelsen betraktelig for å støtte lange tidsperioder.
// 5 minutter ved 512Hz = 300s * 512Hz = 153600 samples. Vi runder opp.
#define DEVIATION_NUMSAMPLES 160001

// ENDRING: Lagt til en enum for å gjøre modusen mer lesbar.
enum DeviationMode {
    MODE_SECONDS_DEV,
    MODE_EVENTS_DEV
};

class DEVIATIONOBJ : public BASE_CL
{
	protected:
		float meanaccu,devaccu;
		float samples[DEVIATION_NUMSAMPLES];
		float squares[DEVIATION_NUMSAMPLES];
		float mean,deviation;
        int writepos, added;
        // ENDRING: Ny variabel for å oppdage "kanten" av en ny hendelse.
        float last_value;

	public:
		int interval; // Den kalkulerte verdien (i samples eller antall)
        // ENDRING: Nye variabler for å håndtere de to modusene.
        int mode;
		int interval_setting;

	DEVIATIONOBJ(int num);

	void make_dialog(void);

	void load(HANDLE hFile);

	void incoming_data(int port, float value);

	void save(HANDLE hFile);
	
	void work(void);

    // ENDRING: En ny, mer generell funksjon for å oppdatere innstillingene.
	void update_settings(int new_value, int new_mode);

	~DEVIATIONOBJ();

	friend LRESULT CALLBACK DeviationDlgHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
       
};