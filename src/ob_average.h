/* -----------------------------------------------------------------------------

   BrainBay  -  OpenSource Biofeedback Software

  MODULE:  OB_AVERAGE.H  declarations for the Averager-Object
  Author:  Chris Veigl


  This Object outputs the Average of n Samples captured from it's input-port

 This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; See the
  GNU General Public License for more details.

  
-------------------------------------------------------------------------------------*/

#define AVERAGE_NUMSAMPLES 160001

// ENDRING: Lagt til en enum for å gjøre modusen mer lesbar.
enum AverageMode {
    MODE_SECONDS,
    MODE_EVENTS
};

class AVERAGEOBJ : public BASE_CL
{
	protected:
		float accumulator;
		float samples[AVERAGE_NUMSAMPLES];
        long interval; // Dette er den *kalkulerte* verdien (i samples eller antall)
        long writepos, added;

	public:
        // ENDRING: Nye variabler for å håndtere de to modusene.
        int mode;               // Vil lagre enten MODE_SECONDS eller MODE_EVENTS.
        int interval_setting;   // Verdien fra slideren (f.eks. 60 sekunder eller 10 hendelser).

	AVERAGEOBJ(int num);

	void session_start(void);
	void session_reset(void);
	void session_pos(long pos);

	void make_dialog(void);

	void load(HANDLE hFile);

	void incoming_data(int port, float value);

	void save(HANDLE hFile);
	
	void work(void);

	~AVERAGEOBJ();

	friend LRESULT CALLBACK AverageDlgHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    
    private:
    
    // ENDRING: En ny, mer generell funksjon for å oppdatere innstillingene.
    void update_settings(int new_value, int new_mode);
};