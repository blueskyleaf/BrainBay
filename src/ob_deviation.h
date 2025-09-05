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

class DEVIATIONOBJ : public BASE_CL
{
	protected:
		float meanaccu,devaccu;
		float samples[DEVIATION_NUMSAMPLES];
		float squares[DEVIATION_NUMSAMPLES];
		float mean,deviation;
        int writepos, added;

	public:
		int interval; // Dette vil nå være antall SAMPLES, kalkulert fra interval_seconds
		int interval_seconds; // ENDRING: Ny variabel for å holde på intervallet i sekunder

	DEVIATIONOBJ(int num);

	void make_dialog(void);

	void load(HANDLE hFile);

	void incoming_data(int port, float value);

	void save(HANDLE hFile);
	
	void work(void);

    // ENDRING: Endret funksjonen til å ta imot sekunder
	void change_interval_seconds(int newinterval_seconds);

	~DEVIATIONOBJ();

	friend LRESULT CALLBACK DEVIATIONDlgHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
       
    
};