#include "WiimoteConnector.h"
#include <iostream>
#include <thread>
#include <functional>
#include <string>

WiimoteConnector::WiimoteConnector() : _myStatus("Initializing")
{
	_isConnected = false;
	_attemptedConnection = true;
	initializeWiimote();



	int i = 0;
	for (; i < MAX_BUTTONS; ++i) {
		buttons.push_back(0);
	}

	i = 0;
	for (; i < 11; ++i) {
		irDots.push_back(0);
	}

	i = 0;
	for (; i < 2; ++i) {
		nunButtons.push_back(0);
	}

	i = 0;
	for (; i < 2; ++i) {
		nunJoystick.push_back(0);
	}

	i = 0;
	for (; i < 3; ++i) {
		gyro.push_back(0);
	}

}

WiimoteConnector::~WiimoteConnector()
{

}

void WiimoteConnector::update()
{

}


void WiimoteConnector::wiiThread(int id)
{

	while (any_wiimote_connected(_wiimotes, MAX_WIIMOTES)) {
		if (!_isConnected) //Important so that we exit the loop when the toggle is turned off
			return;
		if (wiiuse_poll(_wiimotes, MAX_WIIMOTES)) {
			/*
				*    This happens if something happened on any wiimote.
				*    So go through each one and check if anything happened.
				*/
			int i = 0;
			for (; i < MAX_WIIMOTES; ++i) {
				switch (_wiimotes[i]->event) {
				case WIIUSE_EVENT:
					/* a generic event occurred */
					handle_event(_wiimotes[i]);
					break;

				case WIIUSE_STATUS:
					/* a status event occurred */
					handle_ctrl_status(_wiimotes[i]);
					break;

				case WIIUSE_DISCONNECT:
				case WIIUSE_UNEXPECTED_DISCONNECT:
					/* the wiimote disconnected */
					handle_disconnect(_wiimotes[i]);
					break;

				case WIIUSE_READ_DATA:
					/*
						*    Data we requested to read was returned.
						*    Take a look at wiimotes[i]->read_req
						*    for the data.
						*/
					break;

				case WIIUSE_NUNCHUK_INSERTED:
					/*
						*    a nunchuk was inserted
						*    This is a good place to set any nunchuk specific
						*    threshold values.  By default they are the same
						*    as the wiimote.
						*/
						/* wiiuse_set_nunchuk_orient_threshold((struct nunchuk_t*)&wiimotes[i]->exp.nunchuk, 90.0f); */
						/* wiiuse_set_nunchuk_accel_threshold((struct nunchuk_t*)&wiimotes[i]->exp.nunchuk, 100); */
					//printf("Nunchuk inserted.\n");
					break;

				case WIIUSE_CLASSIC_CTRL_INSERTED:
					//printf("Classic controller inserted.\n");
					break;

				case WIIUSE_WII_BOARD_CTRL_INSERTED:
					//printf("Balance board controller inserted.\n");
					break;

				case WIIUSE_GUITAR_HERO_3_CTRL_INSERTED:
					/* some expansion was inserted */
					handle_ctrl_status(_wiimotes[i]);
					//printf("Guitar Hero 3 controller inserted.\n");
					break;

				case WIIUSE_MOTION_PLUS_ACTIVATED:
					//printf("Motion+ was activated\n");
					break;

				case WIIUSE_NUNCHUK_REMOVED:
				case WIIUSE_CLASSIC_CTRL_REMOVED:
				case WIIUSE_GUITAR_HERO_3_CTRL_REMOVED:
				case WIIUSE_WII_BOARD_CTRL_REMOVED:
				case WIIUSE_MOTION_PLUS_REMOVED:
					/* some expansion was removed */
					handle_ctrl_status(_wiimotes[i]);
					//printf("An expansion was removed.\n");
					break;

				default:
					break;
				}
			}
		}
	}
}

void WiimoteConnector::disconnect()
{
	_myStatus = "Disconnecting";
	int found, connected;

	_isConnected = false;
	_attemptedConnection = false;
	wiiuse_set_leds(_wiimotes[0], 0);


	
	if (_wiimoteThread.joinable())
	{
		_isConnected = false;
		_attemptedConnection = false;
		_wiimoteThread.join();
		std::cout << "Thread Closed";
		//_lidarThread.detach();
		//_lidarThread.~thread();
		//std::terminate();
		wiiuse_rumble(_wiimotes[0], 1);
		Sleep(200);
		wiiuse_rumble(_wiimotes[0], 0);
		
	}
	wiiuse_cleanup(_wiimotes, MAX_WIIMOTES);
	if (!any_wiimote_connected(_wiimotes, MAX_WIIMOTES)) {
		_myStatus = "Disconnected";
		_nunStatus = "Disconnected";
	}
	handle_disconnect(_wiimotes[0]);

}

void WiimoteConnector::connect()
{
	_myStatus = "Connecting";
	int found, connected;

	/*
	 *	Initialize an array of wiimote objects.
	 *
	 *	The parameter is the number of wiimotes I want to create.
	 */
	_wiimotes = wiiuse_init(MAX_WIIMOTES);
	/*
	 *	Find wiimote devices
	 *
	 *	Now we need to find some wiimotes.
	 *	Give the function the wiimote array we created, and tell it there
	 *	are MAX_WIIMOTES wiimotes we are interested in.
	 *
	 *	Set the timeout to be 5 seconds.
	 *
	 *	This will return the number of actual wiimotes that are in discovery mode.
	 */
	found = wiiuse_find(_wiimotes, MAX_WIIMOTES, 5);
	if (!found) {
		_isConnected = false;
		_attemptedConnection = true;
		std::cout << "No wiimotes found.\n";
		return;
	}

	/*
	 *	Connect to the wiimotes
	 *
	 *	Now that we found some wiimotes, connect to them.
	 *	Give the function the wiimote array and the number
	 *	of wiimote devices we found.
	 *
	 *	This will return the number of established connections to the found wiimotes.
	 */
	connected = wiiuse_connect(_wiimotes, MAX_WIIMOTES);
	if (connected) {
		_isConnected = true;
		std::cout << "Connected to " << connected << "wiimotes (of " << found << "found).\n";

		wiiuse_set_leds(_wiimotes[0], WIIMOTE_LED_1);
		wiiuse_rumble(_wiimotes[0], 1);
		Sleep(200);
		wiiuse_rumble(_wiimotes[0], 0);
		_myStatus = "Connected";
		_wiimoteThread = std::thread([this] { this->wiiThread(1); });
		//std::thread teste(wiiThread, 1);

	}
	else {
		_isConnected = false;
		_attemptedConnection = true;
		std::cout << "Failed to connect to any wiimote.\n";
		return;
	}

}

std::string WiimoteConnector::getCurrentStatus()
{
	return _myStatus;
}

std::string WiimoteConnector::getCurrentWiimote()
{
	std::string _idStatus;

	if (_isConnected) {
		struct wiimote_t* wm;
		wm = _wiimotes[0];
	//	_idStatus = wiiuse_status(_wiimotes[0]);
		_idStatus = std::to_string(wm->unid);
	}
	else {
		_idStatus = "none";
	}
	return _idStatus;
}


std::string WiimoteConnector::getNunchuckStatus()
{
	return _nunStatus;
}


void WiimoteConnector::initializeWiimote()
{
	/*
	struct wiimote_t* wm;
	wm = _wiimotes[0];

	wiiuse_set_motion_plus(wm, 0);
	wiiuse_motion_sensing(wm, 0);
	wiiuse_set_ir(wm, 0);
	*/
}

short WiimoteConnector::any_wiimote_connected(wiimote** wm, int wiimotes) {
    int i;
    if (!wm) {
        return 0;
	}


	// era wm[i] && WIIMOTE_IS_CONNECTED(wm[i])
    for (i = 0; i < wiimotes; i++) {
		
        if (&wm[i] && _isConnected) {
            return 1;
        }
    }
	

    return 0;
}



/**
 *	@brief Callback that handles an event.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *
 *	This function is called automatically by the wiiuse library when an
 *	event occurs on the specified wiimote.
 */
void WiimoteConnector::handle_event(struct wiimote_t* wm) {


}

/**
 *	@brief Callback that handles a controller status event.
 *
 *	@param wm				Pointer to a wiimote_t structure.
 *	@param attachment		Is there an attachment? (1 for yes, 0 for no)
 *	@param speaker			Is the speaker enabled? (1 for yes, 0 for no)
 *	@param ir				Is the IR support enabled? (1 for yes, 0 for no)
 *	@param led				What LEDs are lit.
 *	@param battery_level	Battery level, between 0.0 (0%) and 1.0 (100%).
 *
 *	This occurs when either the controller status changed
 *	or the controller status was requested explicitly by
 *	wiiuse_status().
 *
 *	One reason the status can change is if the nunchuk was
 *	inserted or removed from the expansion port.
 */
void WiimoteConnector::handle_ctrl_status(struct wiimote_t* wm) {
	printf("\n\n--- CONTROLLER STATUS [wiimote id %i] ---\n", wm->unid);

	printf("attachment:      %i\n", wm->exp.type);
	printf("speaker:         %i\n", WIIUSE_USING_SPEAKER(wm));
	printf("ir:              %i\n", WIIUSE_USING_IR(wm));
	printf("leds:            %i %i %i %i\n", WIIUSE_IS_LED_SET(wm, 1), WIIUSE_IS_LED_SET(wm, 2), WIIUSE_IS_LED_SET(wm, 3), WIIUSE_IS_LED_SET(wm, 4));
	printf("battery:         %f %%\n", wm->battery_level);
}

/**
 *	@brief Callback that handles a disconnection event.
 *
 *	@param wm				Pointer to a wiimote_t structure.
 *
 *	This can happen if the POWER button is pressed, or
 *	if the connection is interrupted.
 */
void WiimoteConnector::handle_disconnect(wiimote* wm) {
	printf("\n\n--- DISCONNECTED [wiimote id %i] ---\n", wm->unid);
}


orient_t WiimoteConnector::getWiimoteOrient() {

	
	struct wiimote_t* wm;
	wm = _wiimotes[0];
	

	if (WIIUSE_USING_ACC(wm)) {
	//	printf("wiimote roll  = %f [%f]\n", wm->orient.roll, wm->orient.a_roll);
	//	printf("wiimote pitch = %f [%f]\n", wm->orient.pitch, wm->orient.a_pitch);
	//	printf("wiimote yaw   = %f\n", wm->orient.yaw);


		//_myStatus = std::to_string(wm->orient.roll);
	}

	//return Vector3(0.0, 0.0, 0.0);
	/*
	if (WIIUSE_USING_ACC(wm)) {

		printf("wiimote roll  = %f [%f]\n", wm->orient.roll, wm->orient.a_roll);
		printf("wiimote pitch = %f [%f]\n", wm->orient.pitch, wm->orient.a_pitch);
		printf("wiimote yaw   = %f\n", wm->orient.yaw);
		float test = wm->orient.roll
			return { wm->orient.roll, wm->orient.roll, wm->orient.roll };
		//	return vec3(wm->orient.roll, wm->orient.roll, wm->orient.roll);
		//return  wm->orient.roll;
	}
	*/
	return wm->orient;
}

void WiimoteConnector::acelerometer(bool accToggle) {
	struct wiimote_t* wm;
	wm = _wiimotes[0];

	if (accToggle) {
		wiiuse_motion_sensing(wm, 1);
	}
	else {
		wiiuse_motion_sensing(wm, 0);
	}
}


void WiimoteConnector::gyroscope(bool accToggle) {
	struct wiimote_t* wm;
	wm = _wiimotes[0];

	if (accToggle) {
		wiiuse_set_motion_plus(wm, 2);
	}
	else {
		wiiuse_set_motion_plus(wm, 0);
	}
}

void WiimoteConnector::irTracking(bool irToggle) {
	struct wiimote_t* wm;
	wm = _wiimotes[0];

	if (irToggle) {
		wiiuse_set_ir(wm, 1);
	}
	else {
		wiiuse_set_ir(wm, 0);
	}
}

int WiimoteConnector::wiimoteButton_A() {
	struct wiimote_t* wm;
	wm = _wiimotes[0];
	if (IS_PRESSED(wm, WIIMOTE_BUTTON_A))
		return 1;
	else
		return 0;
}

vector<int> WiimoteConnector::wiimoteButtons() {
	struct wiimote_t* wm;
	wm = _wiimotes[0];
	buttons[0] = IS_PRESSED(wm, WIIMOTE_BUTTON_A);
	buttons[1] = IS_PRESSED(wm, WIIMOTE_BUTTON_B);
	buttons[2] = IS_PRESSED(wm, WIIMOTE_BUTTON_DOWN);
	buttons[3] = IS_PRESSED(wm, WIIMOTE_BUTTON_UP);
	buttons[4] = IS_PRESSED(wm, WIIMOTE_BUTTON_LEFT);
	buttons[5] = IS_PRESSED(wm, WIIMOTE_BUTTON_RIGHT);
	buttons[6] = IS_PRESSED(wm, WIIMOTE_BUTTON_MINUS);
	buttons[7] = IS_PRESSED(wm, WIIMOTE_BUTTON_PLUS);
	buttons[8] = IS_PRESSED(wm, WIIMOTE_BUTTON_ONE);
	buttons[9] = IS_PRESSED(wm, WIIMOTE_BUTTON_TWO);
	buttons[10] = IS_PRESSED(wm, WIIMOTE_BUTTON_HOME);


	return buttons;
}

vector<float> WiimoteConnector::wiimoteIr() {
	struct wiimote_t* wm;
	wm = _wiimotes[0];

	if (WIIUSE_USING_IR(wm)) {
		int i = 0;
		int ir = 0;
		/* go through each of the 4 possible IR sources */
		for (; i < 4; ++i) {
			/* check if the source is visible */
			if (wm->ir.dot[i].visible) {
				irDots[ir] = wm->ir.dot[i].x;
				irDots[ir+1] = wm->ir.dot[i].y;
				
			//	printf("IR source %i: (%u, %u)\n", i, wm->ir.dot[i].x, wm->ir.dot[i].y);
			}
			else {
				irDots[ir] = 0;
				irDots[ir + 1] = 0;
			}
			ir = ir + 2;
		}
		irDots[8] = wm->ir.x;
		irDots[9] = wm->ir.y;
		irDots[10] = wm->ir.z;
//		printf("IR cursor: (%u, %u)\n", wm->ir.x, wm->ir.y);
//		printf("IR z distance: %f\n", wm->ir.z);
	}


	return irDots;
}


bool WiimoteConnector::nunchuckOn() {
	struct wiimote_t* wm;
	wm = _wiimotes[0];
	if (wm->exp.type == EXP_NUNCHUK || wm->exp.type == EXP_MOTION_PLUS_NUNCHUK) {
		_nunStatus = "Connected";
		return true;
	}
	else {
		_nunStatus = "Disconnected";
		return false;
	}
}

orient_t WiimoteConnector::nunchuckAcc() {
	struct wiimote_t* wm;
	wm = _wiimotes[0];

	if (wm->exp.type == EXP_NUNCHUK || wm->exp.type == EXP_MOTION_PLUS_NUNCHUK) {

		struct nunchuk_t* nc = (nunchuk_t*)&wm->exp.nunchuk;


		return nc->orient;
	}
	else {
		return wm->orient;
	}

}

vector<int> WiimoteConnector::nunchuckButtons() {
	struct wiimote_t* wm;
	wm = _wiimotes[0];

	if (wm->exp.type == EXP_NUNCHUK || wm->exp.type == EXP_MOTION_PLUS_NUNCHUK) {
		struct nunchuk_t* nc = (nunchuk_t*)&wm->exp.nunchuk;
		nunButtons[0] = (IS_PRESSED(nc, NUNCHUK_BUTTON_C));
		nunButtons[1] = (IS_PRESSED(nc, NUNCHUK_BUTTON_Z));
	}

	return nunButtons;
}

vector<float> WiimoteConnector::nunchuckJoystick() {
	struct wiimote_t* wm;
	wm = _wiimotes[0];

	if (wm->exp.type == EXP_NUNCHUK || wm->exp.type == EXP_MOTION_PLUS_NUNCHUK) {
		struct nunchuk_t* nc = (nunchuk_t*)&wm->exp.nunchuk;
		nunJoystick[0] = nc->js.x;
		nunJoystick[1] = nc->js.y;	
	}
	return nunJoystick;
}


vector<float> WiimoteConnector::getWiimoteGyro() {
	struct wiimote_t* wm;
	wm = _wiimotes[0];

	
	if (wm->exp.type == EXP_MOTION_PLUS ||
		wm->exp.type == EXP_MOTION_PLUS_NUNCHUK) {
			gyro[0] = wm->exp.mp.angle_rate_gyro.pitch;
			gyro[1] = wm->exp.mp.angle_rate_gyro.roll;
			gyro[2] = wm->exp.mp.angle_rate_gyro.yaw;

			printf("Motion+ angular rates (deg/sec): pitch:%03.2f roll:%03.2f yaw:%03.2f\n",
				wm->exp.mp.angle_rate_gyro.pitch,
				wm->exp.mp.angle_rate_gyro.roll,
				wm->exp.mp.angle_rate_gyro.yaw);
	}
	return gyro;
}

float WiimoteConnector::wiiBattery() {

	float battery = 0;
	if (_isConnected) {
		struct wiimote_t* wm;
		wm = _wiimotes[0];

		battery = wm->battery_level;

	}
	return battery;
}


void WiimoteConnector::wiiRumble(int rumble) {
	struct wiimote_t* wm;
	wm = _wiimotes[0];

	//wiiuse_toggle_rumble(wm);
	wiiuse_rumble(_wiimotes[0], rumble);


}