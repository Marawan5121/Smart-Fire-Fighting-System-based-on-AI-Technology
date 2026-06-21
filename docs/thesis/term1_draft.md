![A blue and white logo Description automatically
generated](docs/thesis/media/media/image1.png){width="1.8534722222222222in"
height="1.025in"}

**MODERN ACADEMY FOR ENGINEERING AND TECHNOLOGY**

**Department of Electronics and Communication Technology**

**Smart Fire Fighting System based on AI Technology**

**(SFFS)**

The project is elaborated by

Ahmed Ali Abd El-Baky 4220170

Ahmed Mohamed Khodary 4190978

Eman Mohamed Ibrahim 4220494

Marawan Mohamed Ragab 4220258

Manar Hamed Abd El-Wahab 4220540

Nesrin Gamal Mahmoud Elsayed 4220164

Rawnaa Elsherbiny Khalel Elsherbiny 4220536

Supervisor

Dr. Maha Gaber

A graduation project is submitted to the Communication Engineering
Department in partial fulfillment of the requirements for the degree of
Bachelor of Science in Communication Engineering.

January 2025

# **Abstract**

The Smart Fire Fighting System (SFFS) project aims to design and
implement an integrated smart system for building safety and disaster
management, surpassing traditional reaction-based systems. Current
systems suffer from high false alarm rates, an inability to adapt to
different emergency scenarios, and inefficiency in managing evacuations
effectively. This project presents a solution based on the Internet of
Things (IoT) and Artificial Intelligence (AI), utilizing a hybrid
architecture that combines a real-time response controller (ESP32) with
an intelligent processing unit (PC). The system integrates data from a
diverse sensor network (smoke, heat, PIR motion, electrical current, and
thermal cameras) to provide proactive detection of faults before they
escalate into fires. All alarms are visually verified using computer
vision models (YOLOv5) to eliminate false alarms. Advanced
functionalities include approximate localization of trapped individuals
via motion tracking, activation of a dynamic light-based evacuation
system to guide individuals to safe routes, and an advanced
self-diagnostic system to ensure reliability.The SFFS features advanced
safety systems, including the BAMB (Balloon-Assisted Means of Bailout)
escape system. An intelligent control system activates this deployable
emergency escape balloon/chute to provide a safe and rapid exit in
emergency situations. It significantly reduces the risk of falls and
minimizes accidents during mass evacuations, particularly in compromised
structures. This design represents a paradigm shift from simple
\"alerting\" to \"intelligent and preventive management,\" significantly
enhancing the safety of lives and property.

# **Literature Review**

Dr. Arvind Mahalle \[1\], Design and Development of 360 Degree Fire
Protection System. The project objectives, which included designing a
system that could detect fires in all directions and extinguish them
quickly, were successfully met. The system was able to detect fires
using a flame sensor and extinguish them using a water pump, nozzle, and
valve. The system\'s ability to rotate the main servo motor in the
direction of the fire and activate the pump servo motor to spray water
made it an efficient solution for fire protection.

S Muruganantham \[2\], Design and Fabrication of 360 Degree Fire
Protection System, Robots have become out to be an aspect wherein many
human beings have shown their interest and gained a reputation due to
the development of many technologies. Consequently, it has been decided
to design something that may make human existence less difficult and
cozier and the interest of this assessment is to make a \"far flung
managed 360 degree fireplace protection device.\" The proposed \"faraway
controlled 360 diploma fire safety machine\" is designed for
extinguishing hearth in a small floor plan of a residence, workplace, or
shopping mall of precise dimensions with the help of family water and a
water pump.

Prof. V. D. Yadav \[3\], 360 Degree Rotating Fire Protection System,
Usual fire protection systems installed in buildings have the following
disadvantages. They spray small amounts of water from each sprinkler,
which may not be enough to put out the fire. The sprinklers are not
targeted and spray an entire floor or building, ruining computers,
furniture and paperwork. While this sprayer gun can spray water in the
desired quantity only at the fire outbreak point to stop the fire
without ruining the complete office furniture and electronics. This demo
version is made to be remotely controlled from a few meters, but the
future version will operate remotely from the fire department.

Dr. Vishnu Agrawal \[4\], 360 Degree Rotating Fire Protection System,
Fire monitors and sprayers are an amiable and controllable high-capacity
water jet used to deal with large fires. Unlike Fire extinguishers, Fire
Monitors are permanently installed and cannot be moved.

# **Acknowledgement** 

The generous contribution of the individual authors is gratefully
acknowledged. Without their donation of time, energy, and expertise,
this handbook would not be possible. While they are owed a debt that
cannot be paid outright, we trust that the application of their work in
solving fire safety engineering problems worldwide will serve as some
reward for their efforts.

The author would also like to express sincere gratitude to Dr. Maha
Gaber, the supervisor of the Smart Fire Fighting System (SFFS), whose
guidance, continuous support, and valuable insights had a profound
impact on the development of this work. Her academic expertise,
constructive feedback, and dedication significantly contributed to
shaping the system and enhancing its technical and scientific quality.

Finally, the Smart Fire Fighting System (SFFS) is dedicated to my family
and friends, whose unwavering encouragement, patience, and moral support
made the completion of this work possible. Their belief and motivation
were a constant source of strength throughout this journey.

# Contents {#contents .TOC-Heading}

[Abstract [i](#abstract)](#abstract)

[Literature Review [ii](#literature-review)](#literature-review)

[Acknowledgement [iii](#acknowledgement)](#acknowledgement)

[Chapter 1 [5](#chapter-1)](#chapter-1)

[Introduction [5](#introduction)](#introduction)

[1.1 []{dir="rtl"}Overview of Fire and Combustion
[6](#overview-of-fire-and-combustion)](#overview-of-fire-and-combustion)

[1.2 []{dir="rtl"}Challenges and Gaps in Existing Fire Safety Systems
[8](#challenges-and-gaps-in-existing-fire-safety-systems)](#challenges-and-gaps-in-existing-fire-safety-systems)

[1.3. Fire Safety Without Intelligence: A System-Level Gap
[8](#fire-safety-without-intelligence-a-system-level-gap)](#fire-safety-without-intelligence-a-system-level-gap)

[1.4 The Proposed Solution: Our Integrated Smart System
[8](#the-proposed-solution-our-integrated-smart-system)](#the-proposed-solution-our-integrated-smart-system)

[[1]{dir="rtl"}.5. Project Objectives
[9](#project-objectives)](#project-objectives)

[1.6 Thesis Structure [11](#thesis-structure)](#thesis-structure)

[Chapter 2 (General Description of the System Components)
[12](#chapter-2-general-description-of-the-system-components)](#chapter-2-general-description-of-the-system-components)

[2.1 Components of the Fire Protection System
[12](#components-of-the-fire-protection-system)](#components-of-the-fire-protection-system)

[2.1.1 ESP 32 [12](#esp-32)](#esp-32)

[2.1.2 GSM Module [13](#gsm-module)](#gsm-module)

[2.1.3 Flame Sensor [14](#flame-sensor)](#flame-sensor)

[2.1.4 Gas sensor [15](#gas-sensor)](#gas-sensor)

[2.1.5 MPU6050 sensor [19](#mpu6050-sensor)](#mpu6050-sensor)

[2.1.6 channel relay module [19](#_Toc217579490)](#_Toc217579490)

[2.1.7 DC Water Pump [20](#dc-water-pump)](#dc-water-pump)

[2.1.8 Camera AI: [21](#camera-ai)](#camera-ai)

[2.1.[9]{dir="rtl"} Fire alarm buzzer
[21](#fire-alarm-buzzer)](#fire-alarm-buzzer)

[2.1.10 Solar Panel equipment [22](#section)](#section)

[[2]{dir="rtl"}.2 []{dir="rtl"} System Architecture Overview
[25](#_Toc217602689)](#_Toc217602689)

[2.4 Advantages and Disadvantage of Gas System
[29](#advantages-and-disadvantage-of-gas-system)](#advantages-and-disadvantage-of-gas-system)

[Chapter 3 (proposed system and wire connection)
[32](#chapter-3-proposed-system-and-wire-connection)](#chapter-3-proposed-system-and-wire-connection)

[3.1 The SFFS System Implementation and Safety Measures
[32](#the-sffs-system-implementation-and-safety-measures)](#the-sffs-system-implementation-and-safety-measures)

[3.2 Problem Definition [32](#problem-definition)](#problem-definition)

[3.3 []{dir="rtl"}Proposed IoT-Based Solution
[32](#proposed-iot-based-solution)](#proposed-iot-based-solution)

[​3.5 []{dir="rtl"}Implementation Challenges & Barriers
[33](#implementation-challenges-barriers)](#implementation-challenges-barriers)

[3.6 Methodology [33](#methodology)](#methodology)

[3.7 []{dir="rtl"}Mechanical Design and System Structure
[34](#mechanical-design-and-system-structure)](#mechanical-design-and-system-structure)

[3.8 SFFS Manufacturing Process and Hardware Assembly
[34](#sffs-manufacturing-process-and-hardware-assembly)](#sffs-manufacturing-process-and-hardware-assembly)

[3.9 []{dir="rtl"}System Operational Workflow
[34](#system-operational-workflow)](#system-operational-workflow)

[3.10 []{dir="rtl"}SFFS System Architecture and Component Interfacing
[35](#sffs-system-architecture-and-component-interfacing)](#sffs-system-architecture-and-component-interfacing)

[3.12 []{dir="rtl"} Block Operation Sequence (From Detection to
Extinguishing)
[38](#block-operation-sequence-from-detection-to-extinguishing)](#block-operation-sequence-from-detection-to-extinguishing)

[3.13 []{dir="rtl"} Edge Programming and Logic
[39](#edge-programming-and-logic)](#edge-programming-and-logic)

[3.14 Example Code (ESP32 Sensor Node)
[42](#example-code-esp32-sensor-node)](#example-code-esp32-sensor-node)

[Chapter 4 (Hardware and Software Implementations)
[44](#section-1)](#section-1)

[4.1 Flowchart Diagram [45](#flowchart-diagram)](#flowchart-diagram)

[4.2 System Description of the Proposed SFFS
[46](#system-description-of-the-proposed-sffs)](#system-description-of-the-proposed-sffs)

[4.3 System Initialization and Communication Setup
[46](#system-initialization-and-communication-setup)](#system-initialization-and-communication-setup)

[4.4 Continuous Monitoring and Safety Assessment
[46](#continuous-monitoring-and-safety-assessment)](#continuous-monitoring-and-safety-assessment)

[4.5 Earthquake and Electrical Fault Handling
[46](#earthquake-and-electrical-fault-handling)](#earthquake-and-electrical-fault-handling)

[4.6 Fire Detection and Sensor Fusion
[46](#fire-detection-and-sensor-fusion)](#fire-detection-and-sensor-fusion)

[4.7 AI-Based Visual Fire Confirmation
[47](#ai-based-visual-fire-confirmation)](#ai-based-visual-fire-confirmation)

[4.8 Fire Confirmation and Emergency Response
[47](#fire-confirmation-and-emergency-response)](#fire-confirmation-and-emergency-response)

[4.9 Event Logging and System Monitoring
[47](#event-logging-and-system-monitoring)](#event-logging-and-system-monitoring)

[4.10 Smart Fire & Safety System (SFSS)
[47](#smart-fire-safety-system-sfss)](#smart-fire-safety-system-sfss)

[4.11 Software [51](#_Toc217578797)](#_Toc217578797)

[4.11.1 System Configuration & Definitions
[51](#system-configuration-definitions)](#system-configuration-definitions)

[4.11.2 Signal Acquisition & Processing
[52](#signal-acquisition-processing)](#signal-acquisition-processing)

[4.11.3 System Calibration
[54](#system-calibration)](#system-calibration)

[4.11.4Main Logic & Sensor Fusion
[55](#main-logic-sensor-fusion)](#main-logic-sensor-fusion)

[4.11.5Actuation & IoT Communication
[56](#actuation-iot-communication)](#actuation-iot-communication)

[Conclusion [56](#conclusion)](#conclusion)

[APPENDIX A ESP32 Control and Monitoring Source Code
[57](#appendix-a-esp32-control-and-monitoring-source-code)](#appendix-a-esp32-control-and-monitoring-source-code)

[References [67](#references)](#references)

**List of Tables**

[Table 4. 1 Color code [51](#_Toc217578797)](#_Toc217578797)

**Table of Figures**

[Figure 1. 1 Fire Triangle [4](#_Toc217594570)](#_Toc217594570)

[Figure 1. 2 Test room configuration for wireless linkage and sprinkler
system test [8](#_Toc217594571)](#_Toc217594571)

[Figure 1. 3 Block Diagram of Smart Fire Detection
[9](#_Toc217594572)](#_Toc217594572)

[Figure 2. 1 ESP32 development board
[13](#_Toc217602613)](#_Toc217602613)

[Figure 2. 2 SIM800L GSM/GPRS module
[13](#_Toc217602614)](#_Toc217602614)

[Figure 2. 3 flame sensor module [14](#_Toc217602615)](#_Toc217602615)

[Figure 2. 4 MQ-2 [16](#_Toc217602616)](#_Toc217602616)

[Figure 2. 5 MQ-5 [17](#_Toc217602617)](#_Toc217602617)

[Figure 2. 6 MQ-6 [18](#_Toc217602618)](#_Toc217602618)

[Figure 2. 7 MQ-6 [18](#_Toc217602619)](#_Toc217602619)

[Figure 2. 8 MPU6050 sensor [19](#_Toc217602620)](#_Toc217602620)

[Figure 2. 9 channel relay module [20](#_Toc217602621)](#_Toc217602621)

[Figure 2. 10 DC Water Pump [20](#_Toc217602622)](#_Toc217602622)

[Figure 2. 11 Camera AI [21](#_Toc217602623)](#_Toc217602623)

[Figure 2. 12 Fire alarm buzzer [21](#_Toc217602624)](#_Toc217602624)

[Figure 2. 13 Solar Panel [22](#_Toc217602625)](#_Toc217602625)

[Figure 2. 14 Li-ion Battery [23](#_Toc217602626)](#_Toc217602626)

[Figure 2. 15 CN3791 [23](#_Toc217602627)](#_Toc217602627)

[Figure 2. 16 LM2596 [24](#_Toc217602628)](#_Toc217602628)

[Figure 2. 17 Block diagram of fire detection system
[25](#_Toc217602629)](#_Toc217602629)

[Figure 2. 18 Gas suppression system
[29](#_Toc217602630)](#_Toc217602630)

[Figure 3. 1 Block diagram: integrated sensting,AI processing and
Automatic response flow [31](#_Toc217596486)](#_Toc217596486)

[Figure 3. 2 Block Diagram of SFFS [32](#_Toc217596487)](#_Toc217596487)

[Figure 3. 3 Sensor Glove system [33](#_Toc217596488)](#_Toc217596488)

[Figure 3. 4 a) Flowchart of the Bluetooth communication link. (b).
Pairing result between the Bluetooth transceiver.
[34](#_Toc217596489)](#_Toc217596489)

[Figure 4. 1(Flowchart) [42](#_Toc217584908)](#_Toc217584908)

[Figure 4. 2(SFSS) [45](#_Toc217584909)](#_Toc217584909)

[Figure 4. 3(System Configuration & Definitions)
[48](#_Toc217584910)](#_Toc217584910)

[Figure 4. 4(readAdcStabl) [49](#_Toc217584911)](#_Toc217584911)

[Figure 4. 5(Function 2: mpuDangerDetected)
[50](#_Toc217584912)](#_Toc217584912)

[Figure 4. 6(System Calibration) [51](#_Toc217584913)](#_Toc217584913)

[Figure 4. 7(Main Logic & Sensor Fusion)
[52](#_Toc217584914)](#_Toc217584914)

[Figure 4. 8(Actuation & IoT Communication)
[53](#_Toc217584915)](#_Toc217584915)

# **Chapter 1**

# **Introduction**

Nowadays, fire incidents have become a critical issue, which must be
dealt with on time without any unnecessary delay to avoid loss of lives
and belongings. It is considered a fire situation when the monitored
temperature exceeds 50 °C. In critical places such as hospitals,
schools, and banks, personnel\'s arrival time to come for help in fire
hazards is around 15 minutes. The statistics show that there are 475,500
structural fires annually in the United States, causing 2,950 civilian
deaths, 12,775 civilian injuries, and \$7.9 billion in property damage.
According to the National Fire Protection Association (NFPA), two-thirds
of U.S. household fires occur in premises with no working smoke alarms,
alarms with no proper maintenance, or misplaced alarms. The appropriate
allocation of fire alarms with a proactive warning could save lives and
reduce property losses. Particularly, there are many types of fire
alarms as heat detectors and smoke detectors; studying these types helps
to decide which type is more suitable for a home or store. For instance,
heat detectors are classic options when the temperature reaches a
certain level. Heat detectors have a lower false alarm rate but are
still slower in response because the temperature rises slowly \[5\].

# **1.1 []{dir="rtl"}Overview of Fire and Combustion**

[]{dir="rtl"} Fire Triangle

![[]{#_Toc217594570 .anchor}Figure 1. Fire
Triangle](docs/thesis/media/media/image2.png){width="2.3194444444444446in"
height="2.313604549431321in"}

In Figure 1, fire is the visible effect of the process of combustion, a
special type of chemical reaction. It occurs between oxygen in the air
and some sort of fuel. The products from the chemical reaction are
completely different from the starting material.

The fuel must be heated to its ignition temperature for combustion to
occur. The reaction will keep going as long as there is enough heat,
fuel,
and [oxygen](https://www.sciencelearn.org.nz/resources/2701-oxygen).
This is known as the fire triangle.

Combustion is when fuel reacts with oxygen to release heat energy.
Combustion can be slow or fast depending on the amount of oxygen
available. Combustion that results in a flame is very fast and is called
burning. Combustion can only occur between gases.

Chemical reaction in the combustion process. Fuels can be solids,
liquids, or gases. During the chemical reaction that produces fire, fuel
is heated to such an extent that (if not already a gas) it releases
gases from its surface.

Only gases can react in combustion. Gases are made up of molecules
(groups of atoms). When these gases are hot enough, the molecules in the
gases break apart, and fragments of molecules rejoin with oxygen from
the air to make new product molecules, water molecules (H2O), carbon
dioxide molecules (CO2), and other products if burning is not complete
\[6\].

**There are two main causes of fire: Natural and man-made**

**In Natural:**

**Lighting:** Lightning strikes can set trees or houses on fire.
Lightning can enter your home by following wires and pipes that go into
the ground.

**Volcanic Eruption:** It can also travel through metal reinforcing wire
or bars in concrete and explode. Lightning often knocks out power lines
and sends powerful electrical surges through electrical and phone lines.

**Forest Fires:** Naturally occurring wildfires can spark during dry
weather and droughts. In these conditions, normally green vegetation is
present can convert into bone-dry, flammable fuel: strong winds spread
fire quickly, and warm temperatures encourage combustion.

**In man-made**

**Cooking Equipment:** Pots and pans can overheat and cause a fire very
easily if the person cooking gets distracted and leaves cooking
unattended. Always staying in the room or asking someone to watch the
food when cooking on hot plates.

**Heating:** Keep portable heaters at least one metre away from anything
that could easily catch fire, such as furniture, curtains, laundry,
clothes, and even yourself. If you have a furnace, get it inspected once
a year to make sure it is working to safety standards.

**Smoking:** A cigarette that is not put out properly can cause a flame,
as the butt may stay alight for a few hours. It could burst into flames
if it came into contact with flammable materials, such as furniture.

**Electrical Appliances:** An electrical appliance, such as a toaster,
can start a fire if it is faulty or has a frayed cord. A power point
that is overloaded with double adapter plugs can cause a fire from an
overuse of electricity. A power point extension cord can also be a fire
hazard if not used appropriately.

**Candles:** Candles look and smell pretty, but if left unattended, they
can cause a room to easily burst into flames. Keep candles away from any
obviously flammable items such as books and tissue boxes.

**Faulty wiring:** Homes with inadequate wiring can cause fires from
electrical hazards. Some signs that indicate you have bad wiring include
lights dimming when you use another appliance, needing to disconnect one
appliance for another to work, and fuses blowing or the circuit tripping
frequently.

**Flammable Liquids:** If any flammable liquids are in the home or
garage, such as petrol, kerosene, or methylated spirits, keep them away
from heat sources and check the label before storing.

**Short circuits** occur when an unintended path with low resistance
allows a high volume of electrical current to flow. This rapidly
generates extreme heat, often igniting nearby insulation or materials.

**Burning charcoal:** can be dangerous because it retains a high
temperature long after the flames have extinguished. This makes it a
serious fire hazard if not disposed of properly. Additionally, charcoal
releases toxic carbon monoxide gas when used in enclosed spaces.

# **1.2 []{dir="rtl"}Challenges and Gaps in Existing Fire Safety Systems**

Fires and disasters, such as earthquakes, pose a continuous and
escalating threat to lives and infrastructure in modern urban
environments. Despite their importance, building safety systems have not
evolved significantly in recent decades; most still rely on \"dumb
systems.\" These systems are limited to emitting loud, uniform sirens,
often triggered by false alarms (due to steam or dust), which leads to a
state of apathy among occupants. When a real disaster strikes, these
systems fail to provide any effective guidance, may contribute to panic
and chaos, and fail to direct individuals away from danger or assist
rescue teams.

# **1.3. Fire Safety Without Intelligence: A System-Level Gap**

The true gap lies in the complete disconnection between detection
systems, response systems, and evacuation systems. There is no central
\"brain\" connecting them. No system asks: \"Is this real smoke or just
steam?\" \"Where are the occupants located right now?\" \"What is the
safest escape route based on the fire\'s location?\", or \"Is there an
electrical fault that will cause a fire in an hour?\"

# **1.4 The Proposed Solution: Our Integrated Smart System**

To bridge this gap, Smart Fire Fighting System presents the design of a
smart and integrated system for disaster management. Instead of mere
reaction, our system focuses on three core pillars:

Prevention: By monitoring the building\'s vital signs (such as power
draw and anomalous temperatures) to prevent incidents before they
happen.

Verification: By using Artificial Intelligence to analyze visual and
thermal data, we ensure every response is to a genuine event.

Guidance: By transforming chaotic evacuations into organized and
dynamically guided processes.

# **[1]{dir="rtl"}.5. Project Objectives**

Smart Fire Fighting System (SFFS) aims to achieve the following
solution:

- Build a prototype of a system that integrates multiple IoT sensors
  with intelligent AI processing to combine data from different sources
  (temperature, smoke, gas, camera, etc.) and use AI to analyze it,
  which increases detection accuracy and allows smarter decision-making.

- Implement a verification algorithm to confirm fires and eliminate
  false alarms to reduce false alarms caused by dust, steam, cooking
  smoke, or sensor noise, and ensure that the system activates only in
  real fire situations.

- Design and implement a dynamic optical evacuation system that adapts
  to the location of the hazard to guide occupants safely by showing an
  optimal evacuation path that avoids dangerous areas, reducing
  evacuation time, and preventing panic.

- Develop a PIR-based occupancy tracking system to identify the
  locations of potentially trapped individuals to detect human presence
  in rooms or corridors during a fire, enabling rescuers to focus on
  locations where people are most likely to be trapped.

Figure 1.2 Test room configuration for wireless linkage and sprinkler
system test

![[]{#_Toc217594571 .anchor}Figure 1. 2 Test room configuration for
wireless linkage and sprinkler system
test](docs/thesis/media/media/image3.png){alt="A diagram of a house AI-generated content may be incorrect."
width="4.777083333333334in" height="2.4131944444444446in"}

Implement a self-diagnostic system to ensure the system\'s reliability
and continuous operation to automatically check the status of sensors,
power, communication, and actuators, allowing early detection of faults
and ensuring that the fire system is always functional.

In figure 2 shows the location configuration of the sensors and control
panel installed for wireless communication linkage test. A monitoring
program was developed for testing purpose. To test the functionality of
the devices described earlier, smoke and gas were introduced into each
area where corresponding sensors were installed. Once the triggering
substance is detected, wireless signal was received and the fire alarm
was activated. The test has been shown to confirm the wireless
communication capability of the system. In the fire test with a
single-floor three-bedroom residential building, four CO+ smoke sensors
and one LNG/LPG sensor were found to be the optimal configuration. In
such sensor location, configuration wireless communication can be

![[]{#_Toc217594572 .anchor}Figure 1. Block Diagram of Smart Fire
Detection](docs/thesis/media/media/image4.png){alt="A diagram of a system AI-generated content may be incorrect."
width="6.425in" height="3.2222222222222223in"}

established within 7-meter radius \[7\].

Figure 1.3 Block Diagram of Smart Fire Detection

This system integrates multi-level protection in Figure 1.3 by combining
gas/smoke detection, visual AI surveillance, automatic water
suppression, and electrical isolation. Such intelligent setups are
recommended for modern homes and critical facilities to enhance safety
and provide fast, efficient fire response.

The system integrates multiple sensors (temperature, smoke, and flame)
with an ESP32 microcontroller to detect fire conditions in real time.
When a fire is detected, the ESP32 activates an alarm (buzzer and LED)
and triggers a relay module to automatically operate a DC water pump,
which sprays water through a sprinkler head to extinguish the fire
without human intervention. The system can also send data to a laptop
via Wi-Fi for monitoring.

# **1.6 Thesis Structure**

Smart Fire Fighting System (SFFS) based on AI Technology consists of
four chapters:

**Chapter 1** This chapter introduces the fundamentals of fire and
combustion, the causes of fires, and the limitations of current safety
systems. It highlights the need for an intelligent, integrated solution
and presents the proposed Smart Fire Fighting System (SFFS) with its
main objectives.

**Chapter 2** []{dir="rtl"}describes the main hardware components of the
smart fire-fighting system, including the ESP32 controller, power
supply, and sensor layer. It explains how the ESP32 collects data and
centrally controls all devices, supported by computer vision and
wireless communication for real-time detection and response.

**Chapter 3** explains the practical implementation of the smart fire
detection and suppression system, including the wiring of sensors, ESP32
programming, and relay-based control of pumps and alarms. It describes
how sensor data and computer vision are integrated to verify fires,
trigger alerts, and activate water or gas systems using IoT
communication.

**Chapter 4** This chapter presented the complete design and
implementation of the Smart Fire Fighting System (SFFS) based on the
ESP32 platform. It discussed the system architecture, sensor
integration, AI-based fire verification, and multi-level decision-making
process. The chapter also explained the software structure, signal
processing, calibration methods, and actuation mechanisms used to ensure
reliable fire detection and emergency response. Overall, the chapter
demonstrated how sensor fusion and IoT communication enhance system
accuracy, reliability, and real-time monitoring.

# **Chapter 2 (General Description of the System Components)**

# **2.1 Components of the Fire Protection System**

# **2.1.1 ESP 32**

ESP32 comes with an on-chip 32-bit microcontroller with integrated
Wi-Fi + Bluetooth + BLE features that target a wide range of
applications. It is a series of low-power and low-cost devices developed
by Expressive.

**Features of ESP 32**

- ESP-Wroom-32 contains []{dir="rtl"}a low-power Tensilica Xtensa
  Dual-Core 32-bit LX6 microprocessor at 240 MHz: 994.26 CoreMark; 4.14
  CoreMark/MHz

- 448 KB of ROM for booting and core functions.

- 520 KB of on-chip SRAM for data and instructions.

- 4MB of Flash Memory

- 16 KB SRAM in RTC

- Wi-Fi 802.11b/g/n

- Bluetooth v4.2 BR/EDR and Bluetooth LE specifications

![[]{#_Toc217602613 .anchor}Figure 2. ESP32 development
board](docs/thesis/media/media/image5.jpeg){width="6.4in"
height="3.325in"}

# **2.1.2 GSM** **Module**

![[]{#_Toc217602614 .anchor}Figure 2. SIM800L GSM/GPRS
module](docs/thesis/media/media/image6.png){width="6.283333333333333in"
height="2.408333333333333in"}

A GSM (Global System for Mobile Communications) module refers to a
specialized hardware device that utilizes GSM technology to enable
communication capabilities through cellular networks. Incorporating
a [GSM module](https://robocraze.com/collections/gsm-gps-gprs) into an
application allows for bidirectional wireless communication by sending
and receiving both data and voice calls**.**

**Main Uses and Applications**

There are several key applications where GSM modules prove useful:

Communication Capabilities GSM modules core purpose is enabling
communication capabilities by sending and receiving cellular voice calls
and text messages. This allows another system it is integrated with to
leverage the wide-area connectivity of cellular networks to communicate.

# **2.1.3 Flame Sensor**

A flame sensor is a safety device used in gas-burning furnaces and
boilers to detect the presence of a flame. If the system is running (gas
is being released), but no flame is detected, it could lead to a
dangerous buildup of unburned gas. The flame sensor\'s role is to
confirm the existence of a flame when there should be one. If the flame
goes out unexpectedly or doesn\'t ignite when it should, the sensor
signals the control board, which shuts off the gas supply to prevent an
unsafe condition.

![[]{#_Toc217602615 .anchor}Figure 2. flame sensor
module](docs/thesis/media/media/image7.jpeg){alt="KY-026 Flame Sensor | Boson Electronics"
width="6.26636154855643in" height="3.191666666666667in"}

**How Does a Flame Sensor Work?**

Flame sensors are typically made of a conductive metal, like stainless
steel or sometimes a metal rod coated in porcelain, and are positioned
in the path of the burner flame. When the burner is ignited, the flame
makes contact with the sensor. Flames possess a unique property called
\"flame rectification,\" which allows them to act as conductors for
electricity. This means that the flame\'s presence can generate a
microamp-level electrical signal. The flame sensor detects this signal
and communicates to the control board of the appliance. If the control
board doesn\'t receive this microamp signal, it interprets the lack of
signal as an absence of flame, prompting it to shut off the gas supply
as a safety measure.

**Types of Flame Sensors**

- Flame Rectification Sensor (Flame Rods)

- Ultraviolet (UV) Flame Detectors

- Infrared (IR) Flame Detectors

- Visible Light (Video) Flame Detectors

- Thermal (Heat) Detectors

- Ionization Flame Detectors

- Acoustic Flame Detectors

# **2.1.4 Gas sensor** 

A gas detector is a device that detects the presence of gases in a
volume of space, often as part of a safety system. A gas detector can
sound an alarm to operators in the area where the leak is occurring,
allowing them to leave. This type of device is important because there
are many gases that can be harmful to organic life, such as humans or
animals.

Gas detectors can be used to detect combustible, flammable and toxic
gases, and oxygen depletion. This type of device is used widely in
industry and can be found in locations, such as on oil rigs, to monitor
manufacturing processes and emerging technologies such as photovoltaic.
They may be used in firefighting.

**Type of gas sensor**

- Semiconductor Gas sensor

Ex: MQ series(mq2, mq5,mq6,mq7)

- Electrochemical Gas sensor

Ex: Galvanic, voltaic

**MQ-2**

The Grove - Gas Sensor (MQ2) module is useful for gas leakage detection
(home and industry). It is suitable for detecting H2, LPG, CH4, CO,
Alcohol, Smoke or Propane. Due to its high sensitivity and fast response
time, measurements can be taken as soon as possible. The sensitivity of
the sensor can be adjusted with a potentiometer.

![[]{#_Toc217602616 .anchor}Figure 2.
MQ-2](docs/thesis/media/media/image8.jpeg){alt="A gas sensor with a wire on top AI-generated content may be incorrect."
width="3.2680555555555557in" height="1.5625in"}

**MQ-5**

Sensitive material of the MQ-5 gas sensor is SnO2, which with lower
conductivity in clean air. When the target combustible gas exists, the
sensor\'s conductivity is higher along with the gas concentration rises.
The sensor converts change of conductivity to corresponding output
signal of gas concentration.

![[]{#_Toc217602617 .anchor}Figure 2.
MQ-5](docs/thesis/media/media/image9.jpeg){alt="A small metal object with a blue and green board AI-generated content may be incorrect."
width="3.775in" height="2.3703707349081364in"}

MQ-5 gas sensor has high sensitivity to Methane, Propane and Butane
(CH4) and natural gas.

**MQ-6**

The MQ-6 Gas sensor can detect or measure gases like LPG, methane and
butane. The MQ-6 sensor module comes with a Digital Pin which makes this
sensor operate even without a microcontroller and that comes in handy
when you are only trying to detect one particular gas. When it comes to
measuring the gas in ppm the analog pin has to be used. The analog pin
also TTL driven and works on 5V and hence can be used with most common
microcontrollers.

So if you are looking for a sensor to detect or measure gasses like LPG,
butane or methane with or without a microcontroller then this sensor
might be the right choice for you.

![[]{#_Toc217602618 .anchor}Figure 2.
MQ-6](docs/thesis/media/media/image10.jpeg){width="5.016666666666667in"
height="4.95in"}

![[]{#_Toc217602619 .anchor}Figure 2.
MQ-6](docs/thesis/media/media/image11.jpeg){alt="‪Probots MQ-7 Carbon Monoxide Gas Sensor Buy Online India‬‏"
width="2.341666666666667in" height="2.341666666666667in"}

**MQ-[7]{dir="rtl"}**

A carbon monoxide (CO) detector for the diy security system. The carbon
monoxide detector is a semiconductor gas sensor tuned to detect carbon
monoxide. It is in the same family of devices as the smoke detector
sensor, measuring the change in surface conductivity of tin dioxide in
the presence of carbon monoxide. This sensor has a high sensitivity and
fast response time. The sensor\'s output is an analog resistance. The
drive circuit is very simple; all you need to do is power the heater
coil with 5V, add a load resistance, and connect the output to an ADC.

# **2.1.5 MPU6050 sensor**

![[]{#_Toc217602620 .anchor}Figure 2. MPU6050
sensor](docs/thesis/media/media/image12.jpeg){alt="MPU6050 Module"
width="2.4in" height="2.0555555555555554in"}

The MPU6050 sensor module is a complete 6-axis Motion Tracking Device.
It combines a 3-axis Gyroscope, a 3-axis Accelerometer and a Digital
Motion Processor all in a small package. Also, it has an additional
feature of on-chip Temperature sensor. It has an I2C bus interface to
communicate with the microcontrollers.

[]{#_Toc217579490 .anchor}**2.1.6 channel relay module**

A 4-channel relay module is an electronic device that allows a
microcontroller to control four separate electrical circuits. It
essentially acts as a switch, turning high-power devices on or off in
response to a low-power control signal from the microcontroller. Each
channel on the relay module can be independently controlled, enabling
simultaneous control of multiple devices.

![[]{#_Toc217602621 .anchor}Figure 2. channel relay
module](docs/thesis/media/media/image13.jpeg){alt="Relay Module (4 Channels - 5V) – Future Electronics Egypt"
width="2.7777777777777777in" height="1.4916666666666667in"}

# **2.1.7 DC Water Pump**

![[]{#_Toc217602622 .anchor}Figure 2. DC Water
Pump](docs/thesis/media/media/image14.png){width="3.3381944444444445in"
height="3.3381944444444445in"}

A direct current motor-driven pump is used to draw or push water through
the extinguishing system lines. It is the primary mechanical actuator
for water-based suppression.

**Function of pump**

1.  increase pressure

2.  increase flow rate

3.  circulating the fluid

**Type of pumps**

1.  Centrifugal Fire Pumps:

2.  Diesel Fire Pumps

3.  Electric Pumps (electrical fire pumps)

# **2.1.8 Camera AI:**

A camera with onboard processing capabilities for tasks like object or
person detection and fire/smoke visual confirmation. It adds an advanced
layer of visual verification and intelligence.

![[]{#_Toc217602623 .anchor}Figure 2. Camera
AI](docs/thesis/media/media/image15.jpeg){alt="‪AI Cameras: Transforming The Industrial World Through Vision - TechNexion‬‏"
width="1.9430555555555555in" height="1.4402777777777778in"}

# **2.1.[9]{dir="rtl"} Fire alarm buzzer** 

![[]{#_Toc217602624 .anchor}Figure 2. Fire alarm
buzzer](docs/thesis/media/media/image16.png){width="2.095138888888889in"
height="2.095138888888889in"}

is a essential safety devices that are designed to alert occupants of a
building in the event of a fire or other emergency \[8\].

# 

# **2.1.10 Solar Panel equipment**

**Solar Panel**

Solar Panel 18V 20W is a high-efficiency energy solution designed for a
wide range of off-grid applications. It ensures excellent performance
even in low-light conditions. The robust tempered glass covering and
corrosion-resistant aluminum frame offer long-term weather resistance,
making it ideal for both outdoor and mobile installations, such as RVs,
boats, cabins, and remote communication systems.

Lightweight yet strong, the panel features pre-drilled mounting holes
for easy installation and compatibility with various solar system
components. Engineered for reliability and efficiency, this 20W panel
delivers a consistent, sustainable energy output, helping users harness
the sun's power effectively wherever it's needed,

![[]{#_Toc217602625 .anchor}Figure 2. Solar
Panel](docs/thesis/media/media/image17.png){width="2.2916666666666665in"
height="2.2916666666666665in"}

**Li-ion Battery**

![[]{#_Toc217602626 .anchor}Figure 2. Li-ion
Battery](docs/thesis/media/media/image18.png){width="3.84375in"
height="2.323611111111111in"}

This battery is commonly used in solar-powered systems as an energy
storage unit, where it stores electrical energy generated by the solar
cell during daylight hours. The stored energy can then be used to supply
power to electronic systems, such as ESP32-based sensor nodes, when
solar energy is unavailable. Since the battery is unprotected, it must
be used with an appropriate charging and protection circuit, such as a
solar charge.

**CN3791**

![[]{#_Toc217602627 .anchor}Figure 2.
CN3791](docs/thesis/media/media/image19.jpeg){alt="‪HiLetgo CN3791 Solar Charge Controller Board MPPT 1 Cell LiPo Battery Charge 12V Solar Panel Charger Regulator Control Module JST PH2.0 Auto Recharge for Battery with Cables: Buy Online at Best Price‬‏"
width="2.691666666666667in" height="2.033333333333333in"}

The CN3791 is a Maximum Power Point Tracking (MPPT) solar charge
controller IC, often found on small modules, designed to efficiently
charge a single-cell Li-ion/LiPo battery (around 4.2V) from various
solar panel inputs (like 6V, 9V, 12V) by maximizing power extraction,
featuring automatic trickle charging, status indicators, and built-in
protections, making it popular for DIY solar projects and portable power
banks.

**LM2596**

The LM2596 is a DC--DC step-down (buck) voltage regulator widely used in
power management applications. It is designed to efficiently convert a
higher input voltage to a lower, regulated output voltage, with an
efficiency that can exceed 80%. The LM2596 supports an input voltage
range of up to 40 V and can provide an output current of up to 3 A,
making it suitable for powering microcontrollers and electronic modules.

![[]{#_Toc217602628 .anchor}Figure 2.
LM2596](docs/thesis/media/media/image20.png){width="1.71875in"
height="1.71875in"}

In solar-powered systems, the LM2596 is commonly used to regulate the
voltage supplied from a battery or solar charging circuit to a stable
level required by devices such as the ESP32 and its sensors. By
providing a constant and reliable output voltage, the LM2596 helps
protect sensitive electronic components from voltage fluctuations and
ensures stable system operation.[]{#_Toc217602689 .anchor}

**[2]{dir="rtl"}.2 []{dir="rtl"} System** **Architecture Overview**

![[]{#_Toc217602629 .anchor}Figure 2. Block diagram of fire detection
system](docs/thesis/media/media/image21.jpeg){width="5.768055555555556in"
height="3.077777777777778in"}

The system is structured into three distinct layers**:** Perception,
Processing & Logic, and Action, ensuring a modular and scalable approach
to safety management.\
The proposed system is comprised of three interdependent layers, each
fulfilling specific roles in the overall operational flow:

- **Perception Layer:** Responsible for data acquisition from the
  physical environment.

- **Processing & Logic Layer:** Acts as the central intelligence,
  performing data analysis and decision-making.

- **Action Layer:** Executes predetermined responses based on the
  decisions made by the processing layer.

**1. Perception Layer (Sensor)**

This layer constitutes the front-end of the system, responsible for
continuously monitoring the physical environment for potential fire
indicators. It acts as the primary interface between the physical world
and the digital processing units.

**Components:**

- **Camera (Thermal Imaging Sensor):**\
  Provides real-time visual surveillance. Advanced implementations may
  incorporate computer vision algorithms for early detection of smoke
  plumes, flame signatures, or thermal hotspots, offering a vital layer
  of visual verification for potential threats.

- **Smoke Sensor:**\
  Detects the presence of airborne smoke particles. Photoelectric
  sensors are effective for slow-smoldering fires producing large smoke
  particles, while ionization sensors are better suited for fast-flaming
  fires with invisible combustion by-products.

- **Manual Button:**\
  Allows human occupants to manually trigger an alarm upon visual
  confirmation of a fire, providing an immediate response mechanism that
  bypasses automated sensor delays.

- **Heat Detectors:**\
  Respond to either a fixed temperature threshold or a rapid
  rate-of-rise in temperature, suitable for environments where smoke
  detectors may produce false alarms (e.g., kitchens).

- **Gas Leak Detectors:**\
  Critical for areas storing flammable gases, these detect leaks of
  specific gases to prevent explosions or rapid-spreading fires.

**Data Transmission:**\
This phase bridges the gap between raw sensor data and centralized
processing, facilitating efficient and reliable data flow.

- **Microcontroller (ESP32):**\
  ESP32 is a popular choice due to its integrated Wi-Fi and Bluetooth
  capabilities, low power consumption, and processing power. It serves
  as a local processing unit that collects raw analog or digital signals
  from the various sensors, digitizes, filters, and formats this data
  into a standardized protocol suitable for network transmission, acting
  as an intelligent bridge to the main server.

- **MQTT / Wi-Fi:**

  - **Wi-Fi:** Provides the physical layer for wireless network
    connectivity, enabling the microcontroller to communicate with the
    central server.

  - **MQTT (Message Queuing Telemetry Transport):** An efficient,
    publish/subscribe messaging protocol specifically designed for IoT
    devices. Its lightweight nature minimizes bandwidth and power
    consumption, making it ideal for transmitting sensor data reliably.

**2. Processing & Logic Layer**

This layer represents the core intelligence of the system, where data
from multiple sources is aggregated, analyzed, and critical decisions
are made based on predefined algorithms and logic.

**Components:**

- **Cloud / Local Server:**\
  Receives processed data streams from multiple microcontrollers. It
  employs sophisticated \"Decision Logic\" to analyze the received data,
  correlate events from different sensors, and determine the validity
  and severity of a detected threat. It initiates appropriate response
  protocols based on these assessments.

- **Database:**\
  Stores all historical data, including sensor readings, alert triggers,
  system responses, and user interactions. This data is invaluable for
  system performance analysis, incident forensics, compliance reporting,
  and future system optimization through machine learning.

**3. Action Layer**

This layer is responsible for executing the commands issued by the
Processing & Logic Layer, translating digital decisions into tangible
physical actions and notifications.

**Components:**

- **Alert System:**\
  Acts as the central command distributor for all response mechanisms.
  It receives high-level commands from the server and activates the
  appropriate devices (alarms, suppression systems, communication
  modules) with the necessary electrical signals or data packets.

- **Dashboard Screen:**\
  Provides a real-time visual representation of the system\'s status and
  the affected area. Typically displayed on a monitor in a control room,
  it can show floor plans, highlight active fire zones, sensor statuses,
  and system alerts, allowing human operators to monitor and override if
  necessary.

- **Alarm / Siren:**\
  Upon receiving an \"Activate Alarm\" command, it generates loud
  audible warnings (siren) and flashing lights to alert building
  occupants of the imminent danger, prompting immediate evacuation.

- **Mobile App / SMS:**\
  Transmits critical incident notifications via push notifications to a
  dedicated mobile application or through SMS messages to pre-registered
  personnel, ensuring rapid off-site awareness and response
  coordination.

- **Solenoid Valves and Gas Distribution:**\
  After successful completion of detection, validation, and safety
  interlock checks, the solenoid valves controlling the gas cylinders
  are actuated, allowing the gas to be regulated and distributed through
  the main pipeline network to the individual rooms as shown in the
  system layout.

- Uses natural gases (nitrogen, CO₂) to suppress fires[.]{dir="rtl"}

- Lowers oxygen to prevent combustion, safe for brief human
  exposure[.]{dir="rtl"}

![[]{#_Toc217602630 .anchor}Figure 2. Gas suppression
system](docs/thesis/media/media/image22.jpeg){width="5.46875in"
height="3.8645833333333335in"}

Avoids water damage from traditional sprinklers, electrically non
conductive

# **2.4 Advantages and Disadvantage of Gas System**

**Advantages of Gas system**

- Safe for human occupancy:\
  The system uses naturally occurring gases (nitrogen, argon, and a
  small amount of oxygen), which makes the atmosphere breathable during
  evacuation and safe for people inside the protected area.

- Very fast fire suppression:\
  Once heat or smoke is detected, the system releases INWEGEN
  immediately, extinguishing the fire typically within 40 seconds,
  reducing damage and preventing fire spread.

- Environmentally friendly:\
  INERGN has zero ozone depletion potential and zero global warming
  potential, meaning it causes no harm to the environment and complies
  with modern and future environmental standards.

- **No residue left behind:**\
  Unlike water, foam, or chemical extinguishers, gas suppression leaves
  no residue. This eliminates cleanup time and protects sensitive items
  such as documents, electronics, and IT equipment.

- **Safe for equipment and assets:**\
  Because the gas dissipates quickly after discharge and contains no
  corrosive chemicals, it does not damage machines, servers, or
  electrical system

**Disadvantages of Gas System**

• Requires heavy-duty cylinders: The gas is stored in high-pressure
steel cylinders, requiring more materials and a more complex
manufacturing process, thus increasing installation costs.

• Large storage space requirements: The cylinders occupy a significant
amount of space, so the building must have a dedicated storage area,
especially in large systems that require a large number of cylinders.

• Requires regular maintenance: To maintain system effectiveness,
regular inspections and maintenance of the cylinders, piping networks,
and pressure levels are required, increasing costs in the long run.

• Most effective in enclosed spaces only:

Gas systems rely on maintaining a specific gas concentration to
extinguish fires, so they operate most effectively in enclosed or sealed
rooms. They are less effective in large open spaces.

• Higher overall cost: Installing an INERGEN system is more expensive
than many other fire suppression methods, such as sprinklers or foam
systems \[9\].

# **Chapter 3 (proposed system and wire connection)**

# **3.1 The SFFS System Implementation and Safety Measures**

This chapter provides a comprehensive review of existing fire detection
systems and presents the proposed IoT-based fire safety system. The
system integrates sensors, microcontrollers, and wireless communication
to enhance safety measures in facilities**.**

# **3.2 Problem Definition**

Fire outbreaks represent a significant threat to industrial and
residential facilities, often resulting in devastating loss of life and
property. Conventional fire systems frequently suffer from delayed
response times or high false alarm rates, creating a critical gap in
immediate safety measures. These challenges require more advanced,
automated solutions that leverage technology to enable rapid
intervention.​

# **3.3 []{dir="rtl"}Proposed IoT-Based Solution**

​This chapter provides a comprehensive review of existing fire detection
systems and presents the proposed IoT-based fire safety system (SFFS).
The system integrates smart sensors, microcontrollers, and wireless
communication to enhance safety measures in facilities.

​**3.4 []{dir="rtl"}The SFFS operates through the following core
components**

- ​Intelligent Sensing: Utilizing high-precision temperature and smoke
  sensors to monitor environmental changes in real-time.

- Centralized Control & AI: Data is transmitted to a central controller
  that processes inputs using AI technology to identify fire threats
  accurately.

- Immediate Alerting: Upon fire detection, the system sends instant
  emergency notifications directly to employees\' devices, ensuring
  swift evacuation.

- Automated Suppression: The system autonomously activates water-based
  fire extinguishers (Solenoid Valves) to suppress the fire at its early
  stages, minimizing damage.

# ​**3.5 []{dir="rtl"}Implementation Challenges & Barriers**

- Communication Barriers: In extensive facilities, ensuring constant
  wireless connectivity between sensors and the controller is vital to
  prevent communication gaps during emergencies.

- Technological Integration: There is an urgent need for advanced IoT
  technologies that can distinguish between accidental smoke (e.g.,
  cooking or dust) and actual fire hazards to reduce false triggers.

- Access to Information: Providing real-time, accessible alerts to all
  individuals within the facility, including those with disabilities, is
  a primary goal of the SFFS safety protocol

# **3.6 Methodology**

SFFS System Implementation and Safety Measures

The implementation of the Smart Fire Fighting System (SFFS) follows a
multi-disciplinary methodology that integrates mechanical fabrication,
hardware interfacing, and intelligent automation to enhance facility
safety. The process initiates with Structural Fabrication, where
aluminum sheets are shaped and prepared through precision cutting and
drilling to house the internal components. Following the physical
assembly, the Technical Integration phase involves wiring a suite of
sensors including MQ-2 smoke detectors, DS18B20 temperature sensors, and
flame sensors to an ESP32 microcontroller. This core unit is programmed
to process environmental data and infrared signals from a thermal camera
using AI-based pattern recognition.

To bridge the gap between detection and action, a System Interfacing
strategy is applied, utilizing a relay module to control high-power
outputs such as the solenoid valve and fluid pump. This ensures that
once a fire is confirmed, the system can execute a dual-action response:
transmit real-time emergency alerts to an Android application via
Wi-Fi/Bluetooth and simultaneously activate the water extinguishers. To
guarantee Safety and Reliability, the entire implementation undergoes
rigorous stress testing to minimize the lag time between fire sensing
and suppression. This structured approach ensures that the SFFS provides
a robust, automated safety measure capable of protecting both lives and
property through rapid, intelligent intervention.

# **3.7 []{dir="rtl"}Mechanical Design and System Structure**

SolidWorks software is used to produce a 3D schematic diagram of the
integrated chassis for the Smart Fire Fighting System (SFFS). The main
structure consists of a robust protective housing designed to secure the
electronic circuits and the suppression mechanism. The body is 2mm thick
and constructed from an aluminum alloy to provide high resistance to
thermal stress and protect the internal components. The surface is
smooth, painted with a heat-resistant coating, and the alloy sheet can
withstand temperatures of up to 200 °C.

The aluminum body contains strategically placed holes to facilitate the
mounting of various sensors, including flame sensors, smoke sensors
(MQ-2), and temperature sensors (DS18B20) all around the device for
immediate detection.

# **3.8 SFFS Manufacturing Process and Hardware Assembly []{dir="rtl"}**

​The construction of the SFFS device follows a rigorous manufacturing and
assembly procedure designed to ensure structural integrity and
operational efficiency. The process begins with the Fabrication phase,
where aluminum sheets are precisely cut and pressed to form the
device\'s core structural frame. During the Preparation stage, holes are
strategically drilled for sensor mounting, followed by the application
of dry putty to smooth the surface for a professional paint finish. The
Integration and Component Placement phases involve the careful
installation of the system\'s internal hardware, including the ESP32
circuit board, thermal camera, flame and smoke sensors, and the relay
module. A liquid tank and battery are securely housed within the frame,
while a servo motor and shaft are integrated to facilitate the
mechanical movement of the device caps. Each assembly step is
meticulously executed and tested to minimize the response lag between
fire detection and suppression. Once fully assembled, the system
operates as a unified entity: sensing heat and smoke, using the thermal
camera for visual confirmation, and notifying personnel via wireless
alerts while simultaneously triggering the solenoid valve to
automatically deploy the water extinguishers\[9\].

# **3.9 []{dir="rtl"}System Operational Workflow**

**​** The SFFS methodology operates through a seamless, structured flow
that begins with Data Acquisition, where heat and smoke sensors
continuously monitor the environment. This data is then subjected to AI
Analysis as the ESP32 microcontroller processes the information,
integrating infrared input from a thermal camera to identify specific
fire patterns. Upon detection, the system enables wireless communication
by transmitting real-time data and emergency alerts to the Android
application via Wi-Fi or Bluetooth. Finally, the sequence reaches its
Automated Output stage, where the system triggers an audible alarm on
staff devices and simultaneously activates the water extinguishers
through a relay-controlled solenoid valve for immediate fire
suppression.

![[]{#_Toc217596486 .anchor}Figure 3. Block diagram: integrated
sensting,AI processing and Automatic response
flow](docs/thesis/media/media/image23.jpg){width="6.385964566929134in"
height="2.4159722222222224in"}

# **3.10 []{dir="rtl"}SFFS System Architecture and Component Interfacing**

The architecture of the Smart Fire Fighting System (SFFS) is a
multi-layered framework centered on the ESP32 microcontroller, which
orchestrates interactions among various hardware modules. The input
layer consists of a comprehensive sensor suite, including the MQ-2 smoke
sensor for gas detection, the DS18B20 high-precision temperature sensor
for monitoring thermal spikes, and flame sensors that detect light
wavelengths between 760 and 1100 nm. Additionally, a thermal camera is
integrated to provide infrared visualization of heat sources, serving as
a critical diagnostic tool for AI-based fire recognition. These inputs
are interfaced with the ESP32 to provide real-time environmental data.
On the output side, the system uses a relay module that serves as an
automated switch, connecting the microcontroller's logic to the
high-power actuation circuit. Once a fire is confirmed, the ESP32
triggers the relay to activate the solenoid valve and the fluid pump
system. This pump, operating within a 4V--12V range at 1A, ensures the
immediate discharge of fire-terminating fluid from the tank through the
solenoid-controlled path, establishing an efficient and automated fire
suppression sequence\[10\].

![[]{#_Toc217596487 .anchor}Figure 3. Block Diagram of
SFFS](docs/thesis/media/media/image24.emf){width="4.875in"
height="2.6644531933508313in"}

11. ​**Software Part**

​MIT App Inventor is a drag-and-drop visual programming tool used for
designing and building the mobile application for the Smart Fire
Fighting System (SFFS). It allows the developer to focus on the logic of
the fire safety application rather than complex coding syntax, fostering
efficient digital solutions for emergency situations. In our research,
we utilized MIT App Inventor to access mobile features such as
Wi-Fi/Bluetooth connectivity, push notifications, and the loudspeaker
for emergency alerts.

On the mobile device, the application establishes a connection to the
ESP32 controller, receives real-time sensor data (smoke and
temperature), and analyzes these inputs to determine the safety status.
If a fire hazard is recognized, the application triggers a visual and
audible output on the employee\'s device.

![[]{#_Toc217596488 .anchor}Figure 3. Sensor Glove
system](docs/thesis/media/media/image25.emf){width="6.4125in"
height="2.4034722222222222in"}

We used MIT App Inventor to build the following components of the SFFS
app:

Welcoming Screen: Consists of two buttons: \"Monitor Status\" to start
receiving data and \"Exit\" to close the application.

Connection Interface: Built using both \"List Picker\" and
\"Connectivity\" components to establish a stable link between the
mobile and the SFFS hardware.

Real-time Data Display: Developed to show live temperature and smoke
levels directly on the mobile screen.

Emergency Alert System: Programmed to use the mobile\'s loudspeaker to
play a siren sound and display a red \"EMERGENCY\" warning when the AI
logic detects fire.

The application empowers employees with meaningful mobile technology to
enhance their safety within the facility\[11\].

![[]{#_Toc217596489 .anchor}Figure 3. a) Flowchart of the Bluetooth
communication link. (b). Pairing result between the Bluetooth
transceiver.](docs/thesis/media/media/image26.emf){width="5.9518602362204724in"
height="4.1749639107611545in"}

# **3.12 []{dir="rtl"} Block Operation Sequence (From Detection to Extinguishing)**

The sensors (smoke, flame, heat, ultrasonic) continuously transmit their
readings to the ESP32 controller, which integrates these readings while
simultaneously receiving visual confirmation from a camera or laptop
running YOLOv5 to verify the presence of an actual flame or smoke; when
predefined thresholds are exceeded or a fire is visually confirmed, the
software activates Fire Mode, triggering the buzzer and LEDs and sending
alerts via Wi Fi, after which the ESP32 issues commands to the relay
module to activate either water solenoid valve or the gas system
depending on the type of zone and the programmed extinguishing scenario.
At the same time, data from the ultrasonic sensor and other sensors can
also be used to control innovative evacuation lighting systems or to
update the monitoring interface on the laptop to highlight high-risk
areas.

# **3.13 []{dir="rtl"} Edge Programming and Logic**

Sudo-code for SFFS System Implementation

Start LOOP:

Step 1: Initialize all system sensors (Flame sensors, Smoke sensors, and
Temperature sensors) initializeSensors();

Step 2: Initialize the extinguishing actuators (Water pump / CO2
solenoid) and Alarm system initializeActuators();

Step 3: Read real-time data from environment sensors Data =
readSensorData();

Step 4: Convert analog signals to digital values for processing.
processedData = AdcConversion(Data);

Step 5: Check if the values exceed the safety threshold (Fire Detection)
if (fireDetected(processedData))

Step 6: If fire is detected, activate the visual and audible alarms
activateAlarm(status);

Step 7: Execute the extinguishing procedure immediately
ExtinguishingSystem ();

Step 8: Send an emergency notification/location to the mobile app or
fire department via ESP32/GSM sendEmergencyAlert(location);

Step 9: Monitor safety measures (Check if the fire is out or if human
intervention is needed) safetyStatus = checkSafetyProtocol();

Step 10: If the area is safe, deactivate actuators and log the incident
stopExtinguishing();

Go back to the start of the loop END LOOP

The code running on the ESP32 follows these steps:

Establish a Wi-Fi connection, then connect to the MQTT Broker.

Read values and apply the conditional logic locally.

In case of emergency, the microcontroller performs the following:

Activates the local buzzer.

Publishes an MQTT alert message on the Topic: fire/alert.

Sends a HIGH signal to trigger the Relay connected to the Solenoid
Valve.

-Example code for Actuator Node (starts the pump on MQTT command:

// ESP32 subscribes to the pump command topic and drives the relay

#include \<WiFi.h\>

#include \<PubSubClient.h\>

const char\* ssid = \"YOUR_SSID\";

const char\* password = \"YOUR_PASS\";

const char\* mqtt_server = \"192.168.1.100\";

const int mqtt_port = 1883;

const int RELAY_PIN = 26; // GPIO to relay (use transistor/optocoupler)

WiFiClient espClient;

PubSubClient client(espClient);

void callback(char\* topic, byte\* payload, unsigned int length) {

String msg;

for (unsigned int i = 0; i \< length; i++) msg += (char)payload\[i\];

Serial.printf(\"Got topic: %s, msg: %s\\n\", topic, msg.c_str());

if (String(topic) == \"building/actuator/pump1/command\") {

if (msg == \"ON\") {

digitalWrite(RELAY_PIN, HIGH); // adjust depending on relay active state

client.publish(\"building/actuator/pump1/status\",\"ON\");

} else if (msg == \"OFF\") {

digitalWrite(RELAY_PIN, LOW);

client.publish(\"building/actuator/pump1/status\",\"OFF\");

}

}

}

void reconnectMQTT() {

while (!client.connected()) {

if (client.connect(\"esp32_actuator1\")) {

client.subscribe(\"building/actuator/pump1/command\");

} else {

delay(2000);

}

}

}

void setup() {

Serial.begin(115200);

pinMode(RELAY_PIN, OUTPUT);

digitalWrite(RELAY_PIN, LOW);

WiFi.begin(ssid, password);

while (WiFi.status() != WL_CONNECTED) delay(500);

client.setServer(mqtt_server, mqtt_port);

client.setCallback(callback);

}

void loop() {

if (!client.connected()) reconnectMQTT();

client.loop();

}

# **3.14 Example Code (ESP32 Sensor Node)**

Reads temperature + smoke and publishes via MQTT

#include \<WiFi.h\>

#include \<PubSubClient.h\>

#include \"DHT.h\"

const char\* ssid = \"YOUR_SSID\";

const char\* password = \"YOUR_PASS\";

const char\* mqtt_server = \"192.168.1.100\";

#define DHTPIN 15

#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

const int MQ_PIN = 34;

WiFiClient espClient;

PubSubClient client(espClient**);**

unsigned long lastMessage = 0;

void setup() {

Serial.begin(115200);

dht.begin();

WiFi.begin(ssid, password);

while (WiFi.status() != WL_CONNECTED) delay(500);

client.setServer(mqtt_server, 1883);

}

void reconnect() {

while (!client.connected()) {

client.connect(\"esp32_sensor1\");

delay(2000);

}

}

void loop() {

if (!client.connected()) reconnect();

client.loop();

if (millis() - lastMessage \> 10000) {

lastMessage = millis();

float temp = dht.readTemperature();

float hum = dht.readHumidity();

int smoke = analogRead(MQ_PIN);

char buffer\[64\];

snprintf(buffer, sizeof(buffer), \"%.2f\", temp);

client.publish(\"building/floor1/roomA/sensor1/temperature\", buffer);

snprintf(buffer, sizeof(buffer), \"%d\", smoke);

client.publish(\"building/floor1/roomA/sensor1/smoke\", buffer);

Serial.printf(\"Temp: %.2f Smoke: %d\\n\", temp, smoke);

}

# 

# 

# 

# 

# 

# 

# 

# 

# 

# 

# 

# 

# 

# **Chapter 4 (Hardware and Software Implementations)**

# **4.1 Flowchart Diagram**

![[]{#_Toc217584908 .anchor}Figure 4.
(Flowchart)](docs/thesis/media/media/image27.jpeg){alt="A diagram of a company AI-generated content may be incorrect."
width="6.596754155730534in" height="7.875in"}

##  **4.2 System Description of the Proposed SFFS**

Figure (4.1) illustrates the proposed Smart Fire Fighting System (SFFS)
developed for intelligent fire detection and multi-mode fire
suppression. The system is centered on an ESP32 microcontroller, which
continuously supervises multiple environmental sensors to monitor
ambient conditions. The acquired sensor data are compared with
predefined threshold values to assess the severity and criticality of
potential fire-related events.

## **4.3 System Initialization and Communication Setup**

Upon system power-up or reset, the ESP32 initiates execution by
initializing the microcontroller and configuring all connected sensors,
I/O peripherals, and safety actuators. The system then establishes a
Wi-Fi connection to enable real-time data transmission, alarm
notifications, and system status updates to a remote dashboard or mobile
application.

## **4.4 Continuous Monitoring and Safety Assessment**

After initialization, the system enters a continuous monitoring loop in
which sensor data are periodically acquired and processed. The vibration
sensor, external alarm input, and load current sensor are continuously
observed to ensure both mechanical and electrical safety. The measured
vibration level is compared against a predefined threshold to detect
abnormal motion resulting from equipment malfunction or seismic
disturbances. Simultaneously, the load current is evaluated relative to
its rated value to identify overcurrent, overload, or short-circuit
conditions.

## **4.5 Earthquake and Electrical Fault Handling**

In the absence of fire, excessive vibration is interpreted as an
earthquake or severe mechanical disturbance. In this case, the system
immediately disconnects the main power supply to protect equipment and
occupants, closes the gas valves to prevent leakage or explosion, and
opens emergency exits to facilitate safe evacuation. When an overload or
overcurrent condition is detected, the system disconnects non-critical
loads and transmits a maintenance alert to the dashboard or mobile
application for further inspection.

## **4.6 Fire Detection and Sensor Fusion**

The fire detection routine continuously evaluates fire-related sensors,
including the MQ2 smoke and combustible gas sensor, the MQ7 carbon
monoxide sensor, the flame sensor, and the temperature sensor. If all
measured parameters remain below their respective alarm thresholds, the
system continues operating in normal monitoring mode. Once one or more
sensor readings exceed the predefined thresholds, the system transitions
to the fire verification stage.

## **4.7 AI-Based Visual Fire Confirmation**

To reduce false alarms, an AI-based camera module is employed for visual
verification. The camera analyzes the monitored area using computer
vision algorithms to detect flame or smoke patterns. If the sensors are
triggered but the camera confirms no visible fire, the situation is
classified as uncertain, and the system enters a pre-alarm state,
issuing a warning notification and requesting manual verification before
activating suppression mechanisms.

## **4.8 Fire Confirmation and Emergency Response**

When both sensor readings and AI camera analysis confirm the presence of
a fire, the system transitions to the fire-confirmed state. At this
stage, the system activates the fire suppression mechanisms, enables the
ventilation or exhaust system to reduce smoke accumulation, and triggers
audible and visual alarms to alert occupants and emergency personnel.

## **4.9 Event Logging and System Monitoring**

During all critical events, including earthquake detection, overload
conditions, and confirmed fire incidents, immediate alerts are
transmitted to the dashboard or mobile application. All sensor readings,
alarm states, and system responses are logged and stored, enabling
post-event analysis and continuous system performance evaluation.

# **4.10 Smart Fire & Safety System (SFSS)**

![[]{#_Toc217584909 .anchor}Figure 4.
(SFSS)](docs/thesis/media/media/image28.png){width="6.520833333333333in"
height="7.3875in"}

The illustrated system represents a Smart Fire and Safety System based
on the ESP32-WROOM-DA Module microcontroller. The ESP32 serves as the
central processing unit, collecting data from multiple sensors,
analyzing environmental conditions, and controlling several actuators to
support fire detection, safety response, and remote alerting.

At the core of the system is the ESP32-WROOM-DA Module, which provides
Wi-Fi and Bluetooth connectivity in addition to multiple analog,
digital, and communication interfaces. The ESP32 is mounted on a
breadboard and serves as the main controller responsible for reading
sensor inputs, executing decision logic, and driving output devices such
as relays, pumps, alarms, and indicators.

The system uses several MQ gas sensors to detect different types of
hazardous gases. The MQ-2 sensor is used primarily for smoke and
flammable gas detection, making it suitable for identifying early fire
conditions. The MQ-7 sensor is dedicated to carbon monoxide (CO)
detection, which is critical for identifying incomplete combustion and
toxic gas leaks. The MQ-5 sensor is used for detecting LPG and natural
gas, while the MQ-6 sensor is specialized for propane and butane gas
detection. These sensors are connected to the ESP32's analog input pins
and continuously provide voltage levels proportional to gas
concentration.

An IR flame sensor module is included to directly detect the presence of
fire or flame. This sensor provides a digital output to the ESP32,
allowing immediate fire detection even if gas concentration is still
low. Flame detection significantly increases the reliability and
response speed of the system.

To detect physical hazards and abnormal movements, the system integrates
an MPU6050 accelerometer and gyroscope module via the I2C communication
protocol. This sensor enables the system to detect sudden shocks,
vibrations, or abnormal tilting, which may indicate explosions,
equipment falling, or structural instability during fire incidents.

The system also includes an ACS712 current sensor, which is used to
monitor electrical current. This sensor helps detect abnormal power
consumption or short circuits that could lead to overheating or fire
hazards, adding an extra layer of electrical safety.

For response and actuation, a 4-channel relay module is connected to the
ESP32. This relay module electrically isolates and controls high-power
devices such as the small water pump, DC fan, siren buzzer, and LED
strip. The water pump is used for fire suppression, the DC fan assists
in smoke extraction or ventilation, the siren provides an audible alarm,
and the LED strip offers visual warning or emergency lighting.

A servo motor is also included in the system. This motor can be used to
control mechanical actions such as directing a water nozzle, opening
ventilation paths, or adjusting fire suppression mechanisms based on
detected conditions.

For communication and tracking, the system integrates a GSM module and a
GPS module. The GSM module enables the system to send SMS alerts or make
emergency calls when fire or gas leakage is detected, ensuring
notification even in the absence of Wi-Fi. The GPS module provides
real-time location data, which is especially useful for emergency
response and reporting the exact location of the incident.

Power sustainability is supported through a solar cell, which can be
used as an auxiliary or backup power source. This ensures that the
system remains operational during power outages, which are common during
fire emergencies.

All components are interconnected through the breadboard using clearly
routed power, ground, analog, digital, and communication lines. The
ESP32 continuously monitors sensor data, processes safety conditions,
and activates alarms, actuators, and communication modules when
dangerous situations are detected.

In summary, the illustrated setup demonstrates a multi-layered smart
fire and safety system that combines environmental sensing, physical
monitoring, automated response, and remote communication. The
integration of gas sensors, flame detection, motion sensing, actuation,
and communication modules makes the system robust, reliable, and
suitable for smart buildings, industrial safety, and advanced fire
prevention applications.

**Color Code Table**

  --------------------------------------------------------------
   **Color**       **Function**             **Used For**
  ----------- ---------------------- ---------------------------
      Red          Power (VCC)            Sensors, modules

     Black         Ground (GND)            All components

    Yellow         Analog input          Gas, smoke sensors

     Green         Digital I/O              Relay, buzzer

     Blue         I²C (SDA/SCL)                MPU6050

    Purple         UART (TX/RX)               GSM, GPS

    Orange             PWM                   Servo, pump
  --------------------------------------------------------------

  : []{#_Toc217578797 .anchor}Table 4. Color code

# **4.11 Software**

## **4.11.1 System Configuration & Definitions**

![[]{#_Toc217584910 .anchor}Figure 4. 3(System Configuration &
Definitions)](docs/thesis/media/media/image29.jpeg){alt="A screenshot of a computer program AI-generated content may be incorrect."
width="4.9032841207349085in" height="3.658134295713036in"}

**Explanation:**

\"This module establishes the environment for the system. It imports
the BlynkSimpleEsp32 library for IoT connectivity and Wire.h for I2C
communication with the MPU6050 accelerometer.

A critical design choice was mapping all analog sensors (MQ series)
to **ADC1 pins** (GPIO 32-39). The ESP32 architecture restricts the use
of ADC2 when the Wi-Fi module is active. Using ADC1 ensures reliable
sensor data acquisition during real-time data transmission.\"

## **4.11.2 Signal Acquisition & Processing**

![[]{#_Toc217584911 .anchor}Figure 4.
(readAdcStabl)](docs/thesis/media/media/image30.png){alt="A screen shot of a computer program AI-generated content may be incorrect."
width="3.865236220472441in" height="2.5940594925634297in"}

**Explanation:**

![\"To mitigate electrical noise inherent in analog sensors, this
function implements a **Moving Average Filter**. It captures \$N=8\$
consecutive samples and calculates the arithmetic mean, providing a
smoothed digital representation of the gas
concentration.\"](docs/thesis/media/media/image31.png){alt="A screenshot of a computer program AI-generated content may be incorrect."
width="4.336632764654418in" height="2.7364271653543306in"}

[]{#_Toc217584912 .anchor}Figure 4. 5(Function 2: mpuDangerDetected)

**Explanation:**

\"This function processes the inertial data from the MPU6050. It
calculates the total acceleration vector magnitude using the formula:

$$|A| = \sqrt{A_{x}^{2} + A_{y}^{2} + A_{z}^{2}}$$

If the magnitude exceeds the defined threshold (2.5g), a physical impact
or shock is registered. Additionally, it calculates the tilt angle
relative to the gravity vector to detect if the robot/system has tipped
over.\"

## **4.11.3 System Calibration**

![[]{#_Toc217584913 .anchor}Figure 4. 6(System
Calibration)](docs/thesis/media/media/image32.jpeg){alt="A screenshot of a computer program AI-generated content may be incorrect."
width="5.038553149606299in" height="2.8392465004374454in"}

**Explanation:**

\"MQ gas sensors are sensitive to ambient temperature and humidity. To
ensure accuracy, the system does not use fixed absolute thresholds.
Instead, it performs a **calibration routine** during the boot sequence
to determine the \'clean air\' baseline. The alarm logic is subsequently
based on a **differential value** (Current Reading - Baseline), making
the system adaptable to different environments.\"

## **4.11.4Main Logic & Sensor Fusion**

![[]{#_Toc217584914 .anchor}Figure 4. 7(Main Logic & Sensor
Fusion)](docs/thesis/media/media/image33.png){alt="A screenshot of a computer program AI-generated content may be incorrect."
width="4.974358048993876in" height="3.184207130358705in"}

**Explanation:**

\"This function acts as the central processing task. It employs **Sensor
Fusion** by combining data from gas sensors, optical flame detectors,
and the accelerometer.

The decision-making process utilizes logical **OR gates**:
if *any* single threat (Gas, Fire, or Physical Instability) is detected,
the danger flag is raised. This prioritizes safety by ensuring the
system triggers even if only one sensor detects a hazard.\"

## **4.11.5Actuation & IoT Communication** 

![A screen shot of a computer
screen](docs/thesis/media/media/image34.jpeg){width="4.7572364391951005in"
height="3.0396041119860016in"}

[]{#_Toc217584915 .anchor}Figure 4. 8(Actuation & IoT Communication)

**Explanation:**

\"This module manages the physical response to a threat. It
utilizes **Non-Blocking Timers** (based on millis()) instead of
blocking delay() functions.

1.  **Hysteresis:** The pump remains active for a minimum duration
    (MIN_PUMP_ON_MS) to ensure the fire is fully extinguished, even if
    the sensor reading fluctuates.

2.  **Rate Limiting:** To prevent flooding the user\'s phone with
    notifications, a \'Cool-down\' timer prevents sending multiple API
    calls to the Blynk server within the same minute.\"

# **Conclusion**

In conclusion, this thesis presents a comprehensive engineering design
and proposal for a Smart Fire Detection & Extinguishing System. The
primary objective of this project was to explore and utilize Internet of
Things (IoT) and Embedded Systems technologies to provide effective
solutions for safety and disaster management challenges.

The proposed design relies on the ESP32 microcontroller as the central
processing unit, incorporating a carefully selected array of sensors (MQ
Series, Flame Sensor, and MPU6050) to ensure broad coverage of potential
hazards. Furthermore, the theoretical study and engineering schematics
demonstrated the system\'s capability to seamlessly integrate with
positioning technologies (GPS) and telecommunications (GSM) to guarantee
rapid response times.

This research lays the foundation for a low-cost, high-efficiency safety
system. The feasibility study and software architecture have proven the
system\'s readiness for practical implementation, serving as a scalable
model for future smart security systems.

Future Recommendations

Although the proposed design offers a comprehensive solution for home
safety, there are vast horizons for future development to integrate this
system into a broader security ecosystem. We suggest the following:

Smart Cities Integration: Developing communication protocols to link the
system directly with smart city infrastructure and Civil Defense central
control rooms. This would allow for the transmission of automatic
distress signals immediately upon detection, eliminating the need for
human intervention.

Intelligent Rescue Guidance: Enhancing the system to include Occupancy
Detection using advanced motion and thermal sensors. The goal is to
precisely locate rooms containing trapped individuals and transmit
detailed building layouts to firefighters, thereby facilitating
evacuation operations and minimizing human casualties.

# **APPENDIX A ESP32 Control and Monitoring Source Code**

This appendix presents the complete ESP32 source code used for
implementing the Smart Fire Fighting System based on AI Technology. The
code integrates gas sensors, flame detection, motion sensing, and
IoT-based alerting using the Blynk platform.

+-------------------------------------------------------------------+
| /\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*   |
|                                                                   |
|   ESP32 Firefighting System                                       |
|                                                                   |
|   - MQ2, MQ5, MQ6, MQ7 (Analog via ADC1)                          |
|                                                                   |
|   - Flame sensor (Digital)                                        |
|                                                                   |
|   - MPU6050 (I2C)                                                 |
|                                                                   |
|   - Green LED = Safe                                              |
|                                                                   |
|   - Red LED + Pump = Alarm                                        |
|                                                                   |
|   - Blynk IoT Notifications via Blynk.logEvent()                  |
|                                                                   |
| \*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*/ |
|                                                                   |
| #define BLYNK_TEMPLATE_ID   \"PUT_YOUR_TEMPLATE_ID_HERE\"         |
|                                                                   |
| #define BLYNK_TEMPLATE_NAME \"FireFightingSystem\"                |
|                                                                   |
| #define BLYNK_AUTH_TOKEN    \"PUT_YOUR_DEVICE_AUTH_TOKEN_HERE\"   |
|                                                                   |
| #include \<WiFi.h\>                                               |
|                                                                   |
| #include \<BlynkSimpleEsp32.h\>                                   |
|                                                                   |
| #include \<Wire.h\>                                               |
|                                                                   |
| #include \<Adafruit_MPU6050.h\>                                   |
|                                                                   |
| #include \<Adafruit_Sensor.h\>                                    |
|                                                                   |
| char ssid\[\] = \"YOUR_WIFI_NAME\";                               |
|                                                                   |
| char pass\[\] = \"YOUR_WIFI_PASSWORD\";                           |
|                                                                   |
| // \-\-\-\-\-\-\-\-\-- PIN DEFINITIONS (ESP32 WROOM 32 / 38-pin   |
| board) \-\-\-\-\-\-\-\-\--                                        |
|                                                                   |
| const int PIN_MQ2   = 36; // ADC1 (VP) - input only               |
|                                                                   |
| const int PIN_MQ5   = 39; // ADC1 (VN) - input only               |
|                                                                   |
| const int PIN_MQ6   = 34; // ADC1 - input only                    |
|                                                                   |
| const int PIN_MQ7   = 35; // ADC1 - input only                    |
|                                                                   |
| const int PIN_FLAME_DO   = 27; // Digital input                   |
|                                                                   |
| const int PIN_PUMP_RELAY = 26; // Digital output (relay IN)       |
|                                                                   |
| const int PIN_LED_GREEN  = 17; // Digital output                  |
|                                                                   |
| const int PIN_LED_RED    = 16; // Digital output                  |
|                                                                   |
| // I2C for MPU6050 (matches common ESP32 pinout)                  |
|                                                                   |
| const int I2C_SDA = 21;                                           |
|                                                                   |
| const int I2C_SCL = 22;                                           |
|                                                                   |
| // \-\-\-\-\-\-\-\-\-- SETTINGS / THRESHOLDS (TUNE THESE)         |
| \-\-\-\-\-\-\-\-\--                                               |
|                                                                   |
| const bool FLAME_ACTIVE_LOW = true;   // Many LM393 flame modules |
| output LOW when flame detected                                    |
|                                                                   |
| // MQ sensors: we use a baseline + delta threshold method         |
| (simple + practical)                                              |
|                                                                   |
| int mqBaseline2 = 0, mqBaseline5 = 0, mqBaseline6 = 0,            |
| mqBaseline7 = 0;                                                  |
|                                                                   |
| // Increase if you get false alarms; decrease if you want higher  |
| sensitivity                                                       |
|                                                                   |
| const int MQ_DELTA_THRESHOLD = 400;   // raw ADC counts (0..4095) |
|                                                                   |
| // MPU6050 thresholds                                             |
|                                                                   |
| const float SHOCK_G_THRESHOLD = 2.5f; // sudden impact threshold  |
|                                                                   |
| const float TILT_DEG_THRESHOLD = 60.0f; // tilt threshold         |
|                                                                   |
| // Alarm behavior                                                 |
|                                                                   |
| const unsigned long NOTIFY_COOLDOWN_MS = 60UL \* 1000UL; // 1     |
| minute between notifications                                      |
|                                                                   |
| const unsigned long MIN_PUMP_ON_MS     = 10UL \* 1000UL; // keep  |
| pump on at least 10s once alarm triggers                          |
|                                                                   |
| // \-\-\-\-\-\-\-\-\-- GLOBALS \-\-\-\-\-\-\-\-\--                |
|                                                                   |
| BlynkTimer timer;                                                 |
|                                                                   |
| Adafruit_MPU6050 mpu;                                             |
|                                                                   |
| bool alarmActive = false;                                         |
|                                                                   |
| unsigned long alarmStartMs = 0;                                   |
|                                                                   |
| unsigned long lastNotifyMs = 0;                                   |
|                                                                   |
| // Optional: send sensor values to Blynk virtual pins (for        |
| dashboards)                                                       |
|                                                                   |
| const int VPIN_MQ2 = V0;                                          |
|                                                                   |
| const int VPIN_MQ5 = V1;                                          |
|                                                                   |
| const int VPIN_MQ6 = V2;                                          |
|                                                                   |
| const int VPIN_MQ7 = V3;                                          |
|                                                                   |
| const int VPIN_FLAME = V4;                                        |
|                                                                   |
| const int VPIN_ALARM = V5;                                        |
|                                                                   |
| static inline float clampf(float x, float a, float b) { return (x |
| \< a) ? a : (x \> b) ? b : x; }                                   |
|                                                                   |
| void setSafeState() {                                             |
|                                                                   |
|   digitalWrite(PIN_LED_GREEN, HIGH);                              |
|                                                                   |
|   digitalWrite(PIN_LED_RED, LOW);                                 |
|                                                                   |
|   digitalWrite(PIN_PUMP_RELAY, LOW);   // relay OFF (adjust if    |
| your relay is active-low)                                         |
|                                                                   |
| }                                                                 |
|                                                                   |
| void setAlarmState() {                                            |
|                                                                   |
|   digitalWrite(PIN_LED_GREEN, LOW);                               |
|                                                                   |
|   digitalWrite(PIN_LED_RED, HIGH);                                |
|                                                                   |
|   digitalWrite(PIN_PUMP_RELAY, HIGH);  // relay ON (adjust if     |
| your relay is active-low)                                         |
|                                                                   |
| }                                                                 |
|                                                                   |
| int readAdcStable(int pin) {                                      |
|                                                                   |
|   // small averaging to reduce noise                              |
|                                                                   |
|   long sum = 0;                                                   |
|                                                                   |
|   const int N = 8;                                                |
|                                                                   |
|   for (int i = 0; i \< N; i++) {                                  |
|                                                                   |
|     sum += analogRead(pin);                                       |
|                                                                   |
|     delay(2);                                                     |
|                                                                   |
|   }                                                               |
|                                                                   |
|   return (int)(sum / N);                                          |
|                                                                   |
| }                                                                 |
|                                                                   |
| void calibrateGasBaselines() {                                    |
|                                                                   |
|   // Quick baseline calibration (MQ sensors need warmup in real   |
| life)                                                             |
|                                                                   |
|   // For better results, warm up sensors 1-5 minutes before       |
| calibration.                                                      |
|                                                                   |
|   const int SAMPLES = 60;                                         |
|                                                                   |
|   long s2 = 0, s5 = 0, s6 = 0, s7 = 0;                            |
|                                                                   |
|   for (int i = 0; i \< SAMPLES; i++) {                            |
|                                                                   |
|     s2 += readAdcStable(PIN_MQ2);                                 |
|                                                                   |
|     s5 += readAdcStable(PIN_MQ5);                                 |
|                                                                   |
|     s6 += readAdcStable(PIN_MQ6);                                 |
|                                                                   |
|     s7 += readAdcStable(PIN_MQ7);                                 |
|                                                                   |
|     Blynk.run();                                                  |
|                                                                   |
|     delay(20);                                                    |
|                                                                   |
|   }                                                               |
|                                                                   |
|   mqBaseline2 = (int)(s2 / SAMPLES);                              |
|                                                                   |
|   mqBaseline5 = (int)(s5 / SAMPLES);                              |
|                                                                   |
|   mqBaseline6 = (int)(s6 / SAMPLES);                              |
|                                                                   |
|   mqBaseline7 = (int)(s7 / SAMPLES);                              |
|                                                                   |
|   Serial.println(\"MQ baselines calibrated:\");                   |
|                                                                   |
|   Serial.printf(\"MQ2=%d MQ5=%d MQ6=%d MQ7=%d\\n\", mqBaseline2,  |
| mqBaseline5, mqBaseline6, mqBaseline7);                           |
|                                                                   |
| }                                                                 |
|                                                                   |
| bool flameDetected() {                                            |
|                                                                   |
|   int v = digitalRead(PIN_FLAME_DO);                              |
|                                                                   |
|   if (FLAME_ACTIVE_LOW) return (v == LOW);                        |
|                                                                   |
|   return (v == HIGH);                                             |
|                                                                   |
| }                                                                 |
|                                                                   |
| bool mpuDangerDetected(String &mpuReason) {                       |
|                                                                   |
|   sensors_event_t a, g, temp;                                     |
|                                                                   |
|   mpu.getEvent(&a, &g, &temp);                                    |
|                                                                   |
|   // acceleration magnitude in \"g\"                              |
|                                                                   |
|   float ax = a.acceleration.x / 9.80665f;                         |
|                                                                   |
|   float ay = a.acceleration.y / 9.80665f;                         |
|                                                                   |
|   float az = a.acceleration.z / 9.80665f;                         |
|                                                                   |
|   float amag = sqrt(ax\*ax + ay\*ay + az\*az);                    |
|                                                                   |
|   // shock detection                                              |
|                                                                   |
|   if (amag \> SHOCK_G_THRESHOLD) {                                |
|                                                                   |
|     mpuReason = String(\"MPU shock detected (\") + String(amag,   |
| 2) + \"g)\";                                                      |
|                                                                   |
|     return true;                                                  |
|                                                                   |
|   }                                                               |
|                                                                   |
|   // tilt estimation (rough): angle from gravity vector           |
|                                                                   |
|   // We assume \"upright\" means Z close to 1g.                   |
|                                                                   |
|   float zClamped = clampf(az, -1.0f, 1.0f);                       |
|                                                                   |
|   float tiltRad = acos(zClamped);             // 0 when az=1      |
|                                                                   |
|   float tiltDeg = tiltRad \* 180.0f / PI;                         |
|                                                                   |
|   if (tiltDeg \> TILT_DEG_THRESHOLD) {                            |
|                                                                   |
|     mpuReason = String(\"MPU tilt detected (\") +                 |
| String(tiltDeg, 1) + \" deg)\";                                   |
|                                                                   |
|     return true;                                                  |
|                                                                   |
|   }                                                               |
|                                                                   |
|   return false;                                                   |
|                                                                   |
| }                                                                 |
|                                                                   |
| void triggerAlarmOnce(const String &reason) {                     |
|                                                                   |
|   if (!alarmActive) {                                             |
|                                                                   |
|     alarmActive = true;                                           |
|                                                                   |
|     alarmStartMs = millis();                                      |
|                                                                   |
|     setAlarmState();                                              |
|                                                                   |
|   }                                                               |
|                                                                   |
|   // send notification with cooldown                              |
|                                                                   |
|   unsigned long now = millis();                                   |
|                                                                   |
|   if (now - lastNotifyMs \>= NOTIFY_COOLDOWN_MS) {                |
|                                                                   |
|     lastNotifyMs = now;                                           |
|                                                                   |
|     // Event code must exist in Blynk Console -\> Template -\>    |
| Events                                                            |
|                                                                   |
|     Blynk.logEvent(\"fire_alert\", reason);                       |
|                                                                   |
|     Serial.println(\"Blynk event sent: \" + reason);              |
|                                                                   |
|   }                                                               |
|                                                                   |
| }                                                                 |
|                                                                   |
| void updateOutputsAndBlynk(int mq2, int mq5, int mq6, int mq7,    |
| bool flame, bool danger) {                                        |
|                                                                   |
|   // keep pump on minimum time once alarm started                 |
|                                                                   |
|   if (alarmActive) {                                              |
|                                                                   |
|     setAlarmState();                                              |
|                                                                   |
|     unsigned long now = millis();                                 |
|                                                                   |
|     if (!danger && (now - alarmStartMs \>= MIN_PUMP_ON_MS)) {     |
|                                                                   |
|       // Auto-clear after minimum time, if danger is gone         |
|                                                                   |
|       alarmActive = false;                                        |
|                                                                   |
|       setSafeState();                                             |
|                                                                   |
|     }                                                             |
|                                                                   |
|   } else {                                                        |
|                                                                   |
|     if (danger) setAlarmState();                                  |
|                                                                   |
|     else setSafeState();                                          |
|                                                                   |
|   }                                                               |
|                                                                   |
|   // Optional dashboard updates                                   |
|                                                                   |
|   Blynk.virtualWrite(VPIN_MQ2, mq2);                              |
|                                                                   |
|   Blynk.virtualWrite(VPIN_MQ5, mq5);                              |
|                                                                   |
|   Blynk.virtualWrite(VPIN_MQ6, mq6);                              |
|                                                                   |
|   Blynk.virtualWrite(VPIN_MQ7, mq7);                              |
|                                                                   |
|   Blynk.virtualWrite(VPIN_FLAME, flame ? 1 : 0);                  |
|                                                                   |
|   Blynk.virtualWrite(VPIN_ALARM, (danger \|\| alarmActive) ? 1 :  |
| 0);                                                               |
|                                                                   |
| }                                                                 |
|                                                                   |
| void readAllSensorsTask() {                                       |
|                                                                   |
|   int mq2 = readAdcStable(PIN_MQ2);                               |
|                                                                   |
|   int mq5 = readAdcStable(PIN_MQ5);                               |
|                                                                   |
|   int mq6 = readAdcStable(PIN_MQ6);                               |
|                                                                   |
|   int mq7 = readAdcStable(PIN_MQ7);                               |
|                                                                   |
|   bool gasDanger =                                                |
|                                                                   |
|       (mq2 \> mqBaseline2 + MQ_DELTA_THRESHOLD) \|\|              |
|                                                                   |
|       (mq5 \> mqBaseline5 + MQ_DELTA_THRESHOLD) \|\|              |
|                                                                   |
|       (mq6 \> mqBaseline6 + MQ_DELTA_THRESHOLD) \|\|              |
|                                                                   |
|       (mq7 \> mqBaseline7 + MQ_DELTA_THRESHOLD);                  |
|                                                                   |
|   bool flame = flameDetected();                                   |
|                                                                   |
|   String mpuReason = \"\";                                        |
|                                                                   |
|   bool mpuDanger = mpuDangerDetected(mpuReason);                  |
|                                                                   |
|   bool danger = gasDanger \|\| flame \|\| mpuDanger;              |
|                                                                   |
|   if (danger) {                                                   |
|                                                                   |
|     String reason = \"ALERT: \";                                  |
|                                                                   |
|     if (gasDanger)   reason += \"Gas detected; \";                |
|                                                                   |
|     if (flame)       reason += \"Flame detected; \";              |
|                                                                   |
|     if (mpuDanger)   reason += mpuReason + \"; \";                |
|                                                                   |
|     triggerAlarmOnce(reason);                                     |
|                                                                   |
|   }                                                               |
|                                                                   |
|   updateOutputsAndBlynk(mq2, mq5, mq6, mq7, flame, danger);       |
|                                                                   |
|   // Serial debug                                                 |
|                                                                   |
|   Serial.printf(\"MQ2=%d MQ5=%d MQ6=%d MQ7=%d Flame=%d            |
| Alarm=%d\\n\",                                                    |
|                                                                   |
|                 mq2, mq5, mq6, mq7, flame ? 1 : 0, (danger \|\|   |
| alarmActive) ? 1 : 0);                                            |
|                                                                   |
| }                                                                 |
|                                                                   |
| void setup() {                                                    |
|                                                                   |
|   Serial.begin(115200);                                           |
|                                                                   |
|   pinMode(PIN_LED_GREEN, OUTPUT);                                 |
|                                                                   |
|   pinMode(PIN_LED_RED, OUTPUT);                                   |
|                                                                   |
|   pinMode(PIN_PUMP_RELAY, OUTPUT);                                |
|                                                                   |
|   pinMode(PIN_FLAME_DO, INPUT);                                   |
|                                                                   |
|   setSafeState();                                                 |
|                                                                   |
|   // ADC setup                                                    |
|                                                                   |
|   analogReadResolution(12);     // 0..4095                        |
|                                                                   |
|   analogSetAttenuation(ADC_11db); // best for \~0..3.3V range on  |
| ESP32                                                             |
|                                                                   |
|   // I2C + MPU6050                                                |
|                                                                   |
|   Wire.begin(I2C_SDA, I2C_SCL);                                   |
|                                                                   |
|   if (!mpu.begin()) {                                             |
|                                                                   |
|     Serial.println(\"ERROR: MPU6050 not found. Check wiring       |
| SDA=21 SCL=22 and power.\");                                      |
|                                                                   |
|     // still continue (system can work without MPU if needed)     |
|                                                                   |
|   } else {                                                        |
|                                                                   |
|     // Optional tuning                                            |
|                                                                   |
|     mpu.setAccelerometerRange(MPU6050_RANGE_8_G);                 |
|                                                                   |
|     mpu.setGyroRange(MPU6050_RANGE_500_DEG);                      |
|                                                                   |
|     mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);                   |
|                                                                   |
|     Serial.println(\"MPU6050 OK\");                               |
|                                                                   |
|   }                                                               |
|                                                                   |
|   // Blynk connection                                             |
|                                                                   |
|   Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);                      |
|                                                                   |
|   // Baseline calibration (do this when air is \"clean\")         |
|                                                                   |
|   calibrateGasBaselines();                                        |
|                                                                   |
|   // Read sensors periodically                                    |
|                                                                   |
|   timer.setInterval(700L, readAllSensorsTask);                    |
|                                                                   |
| }                                                                 |
|                                                                   |
| void loop() {                                                     |
|                                                                   |
|   Blynk.run();                                                    |
|                                                                   |
|   timer.run();                                                    |
|                                                                   |
| }                                                                 |
+===================================================================+

# **References**

\[1\]. Dr. Arvind Mahalle, Design and Development of 360 Degree Fire
Protection System., International Journal of Innovations in Engineering
and Science, [www.ijies.net](http://www.ijies.net).

\[2\]. S Muruganantham, Design and Fabrication of 360 Degree Fire
Protection System., International Research Journal of education and
Technology, [www.irjet.net](http://www.irjet.net)

\[3\]. Prof. V. D. Yadav , 360 Degree Rotating Fire Protection System.,
International Journal of Advanced Research in Science, Communication and
Technology, www.ijarsct.co.in

\[4\]. Dr. Vishnu Agrawal, 360 Degree Rotating Fire Protection system,
International Journal of Innovative Research in Science, Engineering and
Technology, www.ijirset.com

\[5\] Desima, M.A., Ramli, P., Ramdani, D.F. and Rahman, S., 2017,
November. Alarm system to detect the location of IOT-based public
vehicle accidents. In 2017 International Conference on Computing,
Engineering, and Design (ICCED) (pp. 1-5). IEEE.

\[6\] H. Trada, "Fire fighting system in buildings" \[PowerPoint
slides\]. SlideShare**.**

\[7\] Juhwan Oh , Zhongwei Jiang, and Henry Panganiban. Development of a
Smart Residential Fire Protection System, 2013

\[8\] Future Electronics Egypt,Online. Available:
<https://store.fut-electronics.com/products/gas-sensor-module-mq5-analog-digital>.

\[9\] NFPA, "NFPA 72: National Fire Alarm and Signaling Code," National
Fire Protection Association, 2023.

\[10\] Y. Samkari, K. Guedri, M. Oreijah, S. Munshi, and S. Azam,
\"Designing and testing of a smart firefighting device system
(LAHEEB),\" International Journal of Robotics and Automation (IJRA),
Jun. 2020

\[11\] N. Salema, S. Alharbi, R. Khezendar, and H. Alshami, "Real-time
glove and android application for visual and audible Arabic sign
language translation," in *Proc. 16th International Learning &
Technology Conference*, Jeddah, Saudi Arabia, 2019.
