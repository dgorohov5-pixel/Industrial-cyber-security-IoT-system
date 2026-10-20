# Industrial-cyber-security-IoT-system
Industrial cyber-physical security IoT system for oil refineries
Industrial Safety System Based on Internet of Things (IoT) Technology

	Introduction.
	After completing the 9th grade, I successfully gained admission to a specialized "Rosneft Class"*, where I was tasked with developing a project on the topic: "The Use of IoT Devices to Enhance Personnel Safety at an Oil Refinery." After analyzing the current situation and consulting with my project supervisor—a specialist from a polytechnic university who also works at an oil refinery—I concluded that no existing solutions for this problem were available, and that there was an urgent need to address this challenge.
	My goal was to create a reliable and cost-effective IoT-based alert system for emergency situations—such as the outbreak and containment of fires or the detection of hazardous gas leaks—as well as a system that provides warnings about approaching unmanned aerial vehicles (UAVs). This system is designed to automatically trigger preventive fire-suppression measures for equipment and illuminate UAVs with spotlights to assist mobile engagement teams. It also includes automated perimeter monitoring, tracks the bidirectional movement of individuals through access points, verifies their authorization credentials, and provides timely detection and alerts regarding unauthorized entry or the absence of a valid pass.
	I decided to connect the security system to a monitoring device via a wireless network. This enables the transmission of facility status reports to the Emergency Response Control Center (ERCC) in real time, while also saving the reports for subsequent analysis.

	Operational Algorithm.
	A set of sensors and actuators is connected to an Arduino Uno R3 board. When any of these are triggered, the program executes the corresponding scenarios. Information about these events is transmitted via a Bluetooth (BT) wireless link to a remote device (a smartphone), while simultaneously being recorded in a log file.

	Operational logic:

	Fire protection system.
	The facility is equipped with a flame-detection sensor. In the event of an emergency, the fire alarm is triggered, an audible alert is activated throughout the plant, and the system sends an alarm signal to a remote device (ERCC).
	
	Counter-UAV system.
	An integrated acoustic sensor detects sounds characteristic of approaching aerial vehicles.
If the approach of such objects is detected, the system triggers an alarm and activates an audible alert across the plant grounds. Simultaneously, a spotlight—consisting of bright LEDs mounted on a semicircular servo-driven bracket—is engaged; this bracket rotates 180 degrees laterally to illuminate the upper hemisphere above the target, thereby highlighting the UAV for subsequent destruction and blinding its onboard camera should it be under operator control.
	
	A system for monitoring the presence of hazardous gases in the air.
	An installed gas sensor detects the presence of gases such as methane and propane in the air—indicators of equipment malfunction that pose a risk to the lives and health of employees. In the event of an emergency, the system triggers an alarm and activates a warning signal to facilitate personnel evacuation and the shutdown of the oil refining process.

	Smart activity recognition system for entry points and perimeter monitoring.
	Two ultrasonic distance sensors are installed at the entry point to detect approaches, entries, and exits. The activity-detection logic is as follows: if the sensors are triggered in the sequence 1–2, an "entry" signal is generated, and an RFID access key is required. If the requirement to present an access key is ignored for more than 10 seconds, an alarm is triggered, and an alert regarding unauthorized entry is sent. Conversely, if the sensors are triggered in the sequence 2–1, an "exit" signal is generated, and no access key is required. During an emergency, entry is completely blocked, and any unauthorized entry immediately triggers an alarm.
	Daily statistics on the number of people entering and leaving are maintained.
	The system is capable of responding to multiple emergency situations simultaneously—for instance, the outbreak of a fire or a gas leak. In addition to displaying the security status in real time, the system uses a custom Python script to read alarm codes from a Bluetooth terminal. It then analyzes the type of emergency, as well as its start time, duration, and resolution time—utilizing the `time` library—and saves these logs to a text file.
	
	Future improvement plans.
	Fire suppression.
	There are plans to install a fog generator that simulates the discharge of inert gases—such as IG-541—to contain a fire or to be deployed preemptively in the event of an anticipated UAV attack.

	UAV threat identification.
	In my current project, the microphone-based UAV detection sensor functions merely as a loud-noise detector; since an Arduino-based system cannot perform sound spectrogram analysis, this serves only as a proof of concept. Future iterations will incorporate AI-based sound analysis and identification—either integrated directly with the microphone or by transmitting compressed audio via a radio link to a central data-processing hub.

	Transitioning to a decentralized radio network.
	Currently, the sensors are hardwired to a gateway (based on an Arduino board). Wireless communication exists only between the gateway and the end device.
	My plans to connect remote sensors via a radio link and perform real-time audio analysis of airborne targets using AI are hindered by the hardware limitations of the Arduino platform.
 
	I have begun deploying a LoRa/Meshtastic radio network using ESP32 microcontrollers.
	I am planning to deploy remote audio sensors designed to detect the sound of various types of UAVs in flight. The audio data will be transmitted via a radio network to the plant’s alert center using a low-bitrate audio codec, followed by spectrogram analysis. Alternatively, the audio can be analyzed locally using AI on an ESP32-based system, with an alarm signal subsequently transmitted over the network to the Emergency Response and Command Center (ERCC).
In a real-world operational environment, these audio sensors can be positioned outside the facility at a distance that provides sufficient time for decision-making.
	I also intend to connect remote actuators via the same radio network, enabling the activation of autonomous, battery-powered systems—such as alarm sirens, fire-suppression equipment, and air-defense searchlights.
	Deploying a decentralized wireless network consisting of low-power, highly energy-efficient sensors and controllers—equipped with backup power sources and powered by solar panels—will ensure continued operation during both cellular network outages and broader power grid failures.
	Because these devices are significantly less expensive than existing industrial systems, it becomes feasible to deploy a large number of redundant sensors. This, in turn, allows for the monitoring of a much wider range of potentially hazardous locations and—through the use of specific analysis algorithms—helps reduce the likelihood of false alarms or system failures resulting from individual sensor malfunctions.

*"Rosneft Classes" represent the first stage of a corporate continuous education system—"School – College/University – Enterprise"—established by Rosneft (PJSC NK Rosneft) to develop an external talent pipeline. 
These classes are hosted at top-tier educational institutions across Russia and are attended by 10th and 11th-grade students selected through an individual admissions process. 
The project encompasses over 3,000 students across more than 20 regions where the company operates—stretching from the Samara Region to the Yamal Peninsula and the Far East. The curriculum focuses on preparing future engineers, geologists, and technologists for the oil and gas industry.
Public Joint Stock Company "Oil Company Rosneft" is the leader of the Russian oil industry and one of the largest publicly traded upstream companies in the world. The Company’s core business activities include:
exploration and prospecting for hydrocarbon deposits,
production of oil, gas, and gas condensate,
oil refining and petrochemistry,
sale of oil, gas, and their refined products both within Russia and internationally.
