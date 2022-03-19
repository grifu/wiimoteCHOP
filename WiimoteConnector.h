#ifndef WIIMOTE_H
#define WIIMOTE_H

#define MAX_WIIMOTES				1
#define MAX_BUTTONS					11

#include <thread>
#include <string>
#include <wiiuse.h>
#include <vector>

using std::vector;

class WiimoteConnector
{
public:
	WiimoteConnector();
	~WiimoteConnector();

	void update();
	void connect();
	void disconnect();
	void wiiThread(int id);
	void acelerometer(bool accToggle);
	void gyroscope(bool accToggle);
	void irTracking(bool irToggle);
	void wiiRumble(int rumble);
	vector<int> wiimoteButtons();
	vector<float> wiimoteIr();
	vector<int> nunchuckButtons();
	vector<float> nunchuckJoystick();
	vector<float> getWiimoteGyro();
	float wiiBattery();

	int wiimoteButton_A();

	orient_t getWiimoteOrient();
	orient_t nunchuckAcc();

	bool nunchuckOn();

	std::thread _wiimoteThread;

	std::string getCurrentStatus();
	std::string getNunchuckStatus();
	std::string getCurrentWiimote();
	

private:
	void initializeWiimote();

	short any_wiimote_connected(wiimote** wm, int wiimotes);
	void handle_event(struct wiimote_t* wm);
	void handle_ctrl_status(struct wiimote_t* wm);
	void handle_disconnect(wiimote* wm);

	
	wiimote**			_wiimotes;
	std::string			_myStatus;
	std::string			_nunStatus;
	bool				_isConnected;
	bool				_attemptedConnection;
	vector<int>			buttons;
	vector<int>			nunButtons;
	vector<float>		irDots;
	vector<float>		nunJoystick;
	vector<float>		gyro;
};

#endif