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

// ENDRING: Laget en unik og større makro for bufferstørrelsen.
#define AVERAGE_NUMSAMPLES 160001

class AVERAGEOBJ : public BASE_CL
{
	protected:
		float accumulator;
        // ENDRING: Bruker den nye makroen.
		float samples[AVERAGE_NUMSAMPLES];
        long interval, writepos, added;

	public:
        // ENDRING: Ny variabel for å holde på intervallet i sekunder.
        int interval_seconds;

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
    
    // ENDRING: Endret funksjonen til å ta imot sekunder.
    void change_interval_seconds(int newinterval_seconds);
};