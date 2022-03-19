/* Wiimote CHOP
 * Developed by Luis Grifu (luis.grifu@gmail.com) 
 * Based on Hugo Laliberté code from a Touchdesigner workshop
 * https://github.com/lachose1/wiimoteCHOP-cleanbase
 */

#include "WiimoteCHOP.h"

#include <stdio.h>
#include <string>
#include <string.h>
#include <cmath>
#include <assert.h>
#include <vector>


// These functions are basic C function, which the DLL loader can find
// much easier than finding a C++ Class.
// The DLLEXPORT prefix is needed so the compile exports these functions from the .dll
// you are creating
extern "C"
{

DLLEXPORT
void
FillCHOPPluginInfo(CHOP_PluginInfo *info)
{
	// Always set this to CHOPCPlusPlusAPIVersion.
	info->apiVersion = CHOPCPlusPlusAPIVersion;

	// The opType is the unique name for this CHOP. It must start with a 
	// capital A-Z character, and all the following characters must lower case
	// or numbers (a-z, 0-9)
	info->customOPInfo.opType->setString("Wiimote");

	// The opLabel is the text that will show up in the OP Create Dialog
	info->customOPInfo.opLabel->setString("Wiimote");

	// Information about the author of this OP
	info->customOPInfo.authorName->setString("Luis Grifu");
	info->customOPInfo.authorEmail->setString("luis.grifu@gmail.com");

	// This CHOP can work with 0 inputs
	info->customOPInfo.minInputs = 0;

	// It can accept up to 1 input though, which changes it's behavior
	info->customOPInfo.maxInputs = 1;
}

DLLEXPORT
CHOP_CPlusPlusBase*
CreateCHOPInstance(const OP_NodeInfo* info)
{
	// Return a new instance of your class every time this is called.
	// It will be called once per CHOP that is using the .dll
	return new WiimoteCHOP(info);
}

DLLEXPORT
void
DestroyCHOPInstance(CHOP_CPlusPlusBase* instance)
{
	// Delete the instance here, this will be called when
	// Touch is shutting down, when the CHOP using that instance is deleted, or
	// if the CHOP loads a different DLL
	delete (WiimoteCHOP*)instance;
}

};


WiimoteCHOP::WiimoteCHOP(const OP_NodeInfo* info) : myNodeInfo(info)
{
	myExecuteCount = 0;
	myOffset = 0.0;
	myWiimote = new WiimoteConnector();
	lastWiimoteToggle = false;
	lastAccelerometerToggle = false;
	lastGyroscopeToggle = false;
	lastIrToggle = false;
	lastRumble = false;
	totalChannels = 0;
	isWiimoteOn = false;

}

WiimoteCHOP::~WiimoteCHOP()
{

}

void
WiimoteCHOP::getGeneralInfo(CHOP_GeneralInfo* ginfo, const OP_Inputs* inputs, void* reserved1)
{
	// This will cause the node to cook every frame
	ginfo->cookEveryFrameIfAsked = true;

	// Note: To disable timeslicing you'll need to turn this off, as well as ensure that
	// getOutputInfo() returns true, and likely also set the info->numSamples to how many
	// samples you want to generate for this CHOP. Otherwise it'll take on length of the
	// input CHOP, which may be timesliced.
	ginfo->timeslice = true;

	ginfo->inputMatchIndex = 0;
}

bool
WiimoteCHOP::getOutputInfo(CHOP_OutputInfo* info, const OP_Inputs* inputs, void* reserved1)
{
	// If there is an input connected, we are going to match it's channel names etc
	// otherwise we'll specify our own.
	if (inputs->getNumInputs() > 0)
	{
		return false;
	}
	else
	{
		info->numChannels = NUM_CHANNELS;

		// Since we are outputting a timeslice, the system will dictate
		// the numSamples and startIndex of the CHOP data
		//info->numSamples = 1;
		//info->startIndex = 0

		// For illustration we are going to output 120hz data
		info->sampleRate = 120;
		return true;
	}
}


void
WiimoteCHOP::getChannelName(int32_t index, OP_String *name, const OP_Inputs* inputs, void* reserved1)
{
	std::string chanName = "chan" + std::to_string(index);
	if (index == 0) {
		chanName = "A";
	} else if (index == 1) {
		chanName = "B";
	} else if (index == 2) {
		chanName = "Down";
	} else if (index == 3) {
		chanName = "Up";
	} else if (index == 4) {
		chanName = "Left";
	} else if (index == 5) {
		chanName = "Right";
	} else if (index == 6) {
		chanName = "Minus";
	} else if (index == 7) {
		chanName = "Plus";
	} else if (index == 8) {
		chanName = "One";
	} else if (index == 9) {
		chanName = "Two";
	} else if (index == 10) {
		chanName = "Home";
	} else if (index == 11) {
		chanName = "Roll";
	} else if (index == 12) {
		chanName = "Pitch";
	} else if (index == 13) {
		chanName = "Yaw";
	}  else if (index == 14) {
		chanName = "IrDot1_X";
	}  else if (index == 15) {
		chanName = "IrDot1_Y";
	}  else if (index == 16) {
		chanName = "IrDot2_X";
	}  else if (index == 17) {
		chanName = "IrDot2_Y";
	}  else if (index == 18) {
		chanName = "IrDot3_X";
	}  else if (index == 19) {
		chanName = "IrDot3_Y";
	}  else if (index == 20) {
		chanName = "IrDot4_X";
	}  else if (index == 21) {
		chanName = "IrDot4_Y";
	}  else if (index == 22) {
		chanName = "IrCursor_X";
	}  else if (index == 23) {
		chanName = "IrCursor_Y";
	}  else if (index == 24) {
		chanName = "IrCursor_Z";
	}  else if (index == 25) {
		chanName = "Nunchuck_C";
	}  else if (index == 26) {
		chanName = "Nunchuck_Z";
	}  else if (index == 27) {
		chanName = "Nunchuck_Roll";
	} else if (index == 28) {
		chanName = "Nunchuck_Pitch";
	} else if (index == 29) {
		chanName = "Nunchuck_Yaw";
	} else if (index == 30) {
		chanName = "Nunchuck_Joystick_X";
	} else if (index == 31) {
		chanName = "Nunchuck_Joystick_Y";
	} else if (index == 32) {
		chanName = "Gyro_Pitch";
	} else if (index == 33) {
		chanName = "Gyro_Roll";
	} else if (index == 34) {
		chanName = "Gyro_Yaw";
	} 

	name->setString(chanName.c_str());
}

void
WiimoteCHOP::execute(CHOP_Output* output,
							  const OP_Inputs* inputs,
							  void* reserved)
{
	myExecuteCount++;
	myWiimote->update();
	
	//double	 scale = inputs->getParDouble("Scale");
	isWiimoteOn = inputs->getParInt("Wiimote");
	bool isAccelerometerOn = inputs->getParInt("Accelerometer");
	bool isGyroscopeOn = inputs->getParInt("Gyroscope");
	bool isIrOn = inputs->getParInt("Ir");
	bool isRumble = inputs->getParInt("Rumble");

	totalChannels = output->numChannels;
	if(!lastWiimoteToggle && isWiimoteOn) //If the last status of the toggle was off and now it is on, attempt connection
	{
		myWiimote->connect();	

	}
	else if (lastWiimoteToggle && !isWiimoteOn) {
		myWiimote->disconnect();
	}


	lastWiimoteToggle = isWiimoteOn;

	if (!lastAccelerometerToggle && isAccelerometerOn) {
		myWiimote->acelerometer(true);
	}
	else if (lastAccelerometerToggle && !isAccelerometerOn) {
		myWiimote->acelerometer(false);
	}

	lastAccelerometerToggle = isAccelerometerOn;

	if (!lastGyroscopeToggle && isGyroscopeOn) {
		myWiimote->gyroscope(true);
	}
	else if (lastGyroscopeToggle && !isGyroscopeOn) {
		myWiimote->gyroscope(false);
	}

	lastGyroscopeToggle = isGyroscopeOn;

	
	if (!lastIrToggle && isIrOn) {
		myWiimote->irTracking(true);
	}
	else if (lastIrToggle && !isIrOn) {
		myWiimote->irTracking(false);
	}

	lastIrToggle = isIrOn;
	

	if (!lastRumble && isRumble) {
		myWiimote->wiiRumble(true);
	}
	else if (lastRumble && !isRumble) {
		myWiimote->wiiRumble(false);
	}

	lastRumble = isRumble;


	


	if (isWiimoteOn) {

		float outData[NUM_CHANNELS];

		//outData[3] = (float) myWiimote->wiimoteButton_A();



		int totalButtons = 10;
		// Get button state
		for (int i = 0; i < 11; i++)
		{
			outData[i] = (float)myWiimote->wiimoteButtons()[i];
		}

		// Accelerometer
		outData[totalButtons+1] = 0;
		outData[totalButtons+2] = 0;
		outData[totalButtons+3] = 0;

		if (isAccelerometerOn) {
			orient_t wiimoteOrientation = myWiimote->getWiimoteOrient();
			outData[totalButtons + 1] = wiimoteOrientation.roll;
			outData[totalButtons + 2] = wiimoteOrientation.pitch;
			outData[totalButtons + 3] = wiimoteOrientation.yaw;	// only if IR tracking
		}

		for (int i = 0; i < 11; i++)
		{
			outData[(totalButtons+4)+i] = myWiimote->wiimoteIr()[i];
		}


		if (myWiimote->nunchuckOn()) {
			outData[totalButtons + 15] = (float)myWiimote->nunchuckButtons()[0];
			outData[totalButtons + 16] = (float)myWiimote->nunchuckButtons()[1];

			orient_t nunChuckOrientation = myWiimote->nunchuckAcc();
			outData[totalButtons + 17] = nunChuckOrientation.a_roll;
			outData[totalButtons + 18] = nunChuckOrientation.a_pitch;
			outData[totalButtons + 19] = nunChuckOrientation.yaw;

			outData[totalButtons + 20] = myWiimote->nunchuckJoystick()[0];
			outData[totalButtons + 21] = myWiimote->nunchuckJoystick()[1];

			vector<float> gyroOrientation = myWiimote->getWiimoteGyro();
			outData[totalButtons + 22] = gyroOrientation[0];
			outData[totalButtons + 23] = gyroOrientation[1];
			outData[totalButtons + 24] = gyroOrientation[2];
		
		}
		else {
			for (int i = totalButtons + 15; i < totalButtons + 25; i++)
			{
				outData[i] = 0;
			}
		}
		
		for (int i = 0; i < output->numChannels; i++)
		{
			for (int j = 0; j < output->numSamples; j++)
			{
				output->channels[i][j] = float(outData[i]);
			}
		}
	}


}

int32_t
WiimoteCHOP::getNumInfoCHOPChans(void * reserved1)
{
	// We return the number of channel we want to output to any Info CHOP
	// connected to the CHOP. In this example we are just going to send one channel.
	return 2;
}

void
WiimoteCHOP::getInfoCHOPChan(int32_t index,
										OP_InfoCHOPChan* chan,
										void* reserved1)
{
	// This function will be called once for each channel we said we'd want to return
	// In this example it'll only be called once.

	if (index == 0)
	{
		chan->name->setString("executeCount");
		chan->value = (float)myExecuteCount;
	}

	if (index == 1)
	{
		chan->name->setString("offset");
		chan->value = (float)myOffset;
	}
}

bool		
WiimoteCHOP::getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved1)
{
	infoSize->rows = 6;
	infoSize->cols = 2;
	// Setting this to false means we'll be assigning values to the table
	// one row at a time. True means we'll do it one column at a time.
	infoSize->byColumn = false;
	return true;
}

void
WiimoteCHOP::getInfoDATEntries(int32_t index,
										int32_t nEntries,
										OP_InfoDATEntries* entries, 
										void* reserved1)
{
	char tempBuffer[4096];

	if (index == 0)
	{
		// Set the value for the first column
		entries->values[0]->setString("executeCount");

		// Set the value for the second column
#ifdef _WIN32
		sprintf_s(tempBuffer, "%d", myExecuteCount);
#else // macOS
        snprintf(tempBuffer, sizeof(tempBuffer), "%d", myExecuteCount);
#endif
		entries->values[1]->setString(tempBuffer);
	}

	if (index == 1)
	{
		// Set the value for the first column
		entries->values[0]->setString("outputChannels");

		// Set the value for the second column
#ifdef _WIN32
        sprintf_s(tempBuffer, "%g", (float)totalChannels);
#else // macOS
        snprintf(tempBuffer, sizeof(tempBuffer), "%g", myOffset);
#endif
		entries->values[1]->setString( tempBuffer);
	}

	if (index == 2)
	{
		// Set the value for the first column
		entries->values[0]->setString("Wiimote Status");

		// Set the value for the second column
#ifdef _WIN32

		std::string currentStatus = myWiimote->getCurrentStatus();
#else // macOS
		snprintf(tempBuffer, sizeof(tempBuffer), "%g", myOffset);
#endif
		entries->values[1]->setString(currentStatus.c_str());
	}

	if (index == 3)
	{
		// Set the value for the first column
		entries->values[0]->setString("Nunchuck Status");

		// Set the value for the second column
#ifdef _WIN32
		// sprintf_s(tempBuffer, "%g", myOffset);
		std::string currentStatus = myWiimote->getNunchuckStatus();
#else // macOS
		snprintf(tempBuffer, sizeof(tempBuffer), "%g", myOffset);
#endif
		entries->values[1]->setString(currentStatus.c_str());
	}


	// Battery Level
	if (index == 4)
	{
		// Set the value for the first column
		entries->values[0]->setString("battery level");

		// Set the value for the second column
#ifdef _WIN32
		sprintf_s(tempBuffer, "%g", (myWiimote->wiiBattery()*100));
		
#else // macOS
		snprintf(tempBuffer, sizeof(tempBuffer), "%g", myOffset);
#endif
		std::string chanValue = tempBuffer;
		chanValue = chanValue + "%";

		entries->values[1]->setString(chanValue.c_str());
	}


	if (index == 5)
	{
		// Set the value for the first column
		entries->values[0]->setString("Wiimote Id");

		// Set the value for the second column
#ifdef _WIN32

		std::string currentStatus = myWiimote->getCurrentStatus();
#else // macOS
		snprintf(tempBuffer, sizeof(tempBuffer), "%g", myOffset);
#endif
		entries->values[1]->setString(myWiimote->getCurrentWiimote().c_str());
	}


}

void
WiimoteCHOP::setupParameters(OP_ParameterManager* manager, void *reserved1)
{

	/*
	// Wii_ID_selector for future integration
	{

		


		OP_StringParameter	sp;

		sp.name = "Id";
		sp.label = "Id";

		sp.defaultValue = "Default";

		const char *names[] = { "Default", "Wii2", "Ramp" };
		const char *labels[] = { "Default", "Wii2", "Ramp" };

		// wm->unid

		OP_ParAppendResult res = manager->appendMenu(sp, 3, names, labels);
		assert(res == OP_ParAppendResult::Success);
	}
	*/
	// reset
	{
		OP_NumericParameter	np;

		np.name = "Reset";
		np.label = "Reset";

		OP_ParAppendResult res = manager->appendPulse(np);
		assert(res == OP_ParAppendResult::Success);
	}

	// wiimote on
	{
		OP_NumericParameter	np;

		np.name = "Wiimote";
		np.label = "Wiimote";
		
		OP_ParAppendResult res = manager->appendToggle(np);
		assert(res == OP_ParAppendResult::Success);
	}

	// accelerometer
	{
		OP_NumericParameter	np;

		np.name = "Accelerometer";
		np.label = "Accelerometer";

		OP_ParAppendResult res = manager->appendToggle(np);
		assert(res == OP_ParAppendResult::Success);
	}

	// gyroscope
	{
		OP_NumericParameter	np;

		np.name = "Gyroscope";
		np.label = "Gyroscope";

		OP_ParAppendResult res = manager->appendToggle(np);
		assert(res == OP_ParAppendResult::Success);
	}

	// IR
	{
		OP_NumericParameter	np;

		np.name = "Ir";
		np.label = "Ir";

		OP_ParAppendResult res = manager->appendToggle(np);
		assert(res == OP_ParAppendResult::Success);
	}

	// Rumble
	{
		OP_NumericParameter	np;

		np.name = "Rumble";
		np.label = "Rumble";

		OP_ParAppendResult res = manager->appendToggle(np);
		assert(res == OP_ParAppendResult::Success);
	}

}

void 
WiimoteCHOP::pulsePressed(const char* name, void* reserved1)
{
	if (!strcmp(name, "Reset"))
	{
		myOffset = 0.0;
	}
}

