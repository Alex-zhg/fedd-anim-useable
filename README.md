# FEDD-Animatronic
Firmware and control code for a 3D-printed animatronic head powered by an ESP32. Features a finite state machine with Scanning, Tracking, and Alert modes, VL53L0X time-of-flight sensor for object detection, servo-driven rotation, LED feedback, and telemetry webpage.

Libraries to install: 
- Adafruit VL53L0X library
- ESP32Servo by Kevin Harrington

# Design Overview

This project is the development of an active animatronic figure built around a fully 3D-printed character model. The figure’s head rotates horizontally and reacts dynamically to objects in its environment. The system is powered by an ESP32 microcontroller and operates using a structured Finite State Machine to manage behavior, motion, sensing, and feedback.

The animatronic is designed to feel responsive and lifelike. It continuously scans its surroundings, detects nearby objects, locks onto movement, and escalates its reaction when something approaches too closely. Visual indicators, sound effects, and mechanical motion work together to create expressive behavior.

**Overall System Behavior**  
The head continuously monitors its environment using a forward-facing distance sensor. Based on what it detects, the system transitions between defined behavioral states. Each state controls head motion, LED eye behavior, sound output, and console feedback. The logic is structured using a Finite State Machine to ensure predictable and intelligent transitions.

**Finite State Machine (FSM)**  
SCANNING

- In scanning mode, the head sweeps smoothly from one side to the other and back again, covering its full field of motion. This creates a continuous search pattern.  
- The blue eye LED pulses slowly to indicate idle awareness.  
- The system outputs live angle updates to the console, displaying the current angle.  
- If an object is detected within a defined tracking distance threshold, the system emits a short beep and transitions immediately into tracking mode.

TRACKING (FOUND)

- When an object is detected, the head locks onto the angle where the object was found. It continuously measures distance to maintain alignment and keep the object centered.  
- The red eye LED turns solid to indicate active engagement  
- Live telemetry is sent to the console showing the object’s angle and distance.  
- If the object moves, the head adjusts smoothly to follow it.  
- If the object is lost, the system transitions to the lost tracking state.  
- If the object moves closer than a defined danger threshold, the system escalates into alert mode.

TRACKING (LOST)

- If tracking is interrupted, the system attempts to reacquire the object. The head begins scanning around the last known angle where the object was detected.  
- The red LED pulses slowly to indicate a searching state.  
- Scanning speed increases to improve reacquisition chances.  
- If the object is rediscovered within the tracking threshold, the system returns to active tracking.  
- If a full sweep completes without detecting an object, the system returns to standard scanning mode.

DANGER

- If the object enters a defined danger distance, the system escalates behavior dramatically.  
- The red LED pulses rapidly.  
- Audible beeping increases in speed depending on how close the object is.  
- The figure performs a mechanical or expressive action, such as opening its mouth, speaking, or activating an output mechanism.  
- If the object moves back to a safer distance but remains detectable, the system returns to active tracking.  
- If the object is lost entirely, the system transitions into the lost tracking state.

**Wireless Data Transmission and Live Web Interface**  
The ESP32 hosts an onboard web server that provides real-time system monitoring.

A connected device can access a live webpage that displays:

- Current operating mode  
- Head angle position  
- Measured object distance  
- An animated radar-style sweep visualization

This interface allows live observation of the animatronic’s behavior without needing a direct serial connection. It also provides a foundation for future configuration controls, diagnostics, and remote updates.

**Project Outcome**  
The final result is a responsive animatronic character that scans its environment, detects motion, tracks objects smoothly, reacts dynamically to proximity, and communicates its internal state wirelessly. The system integrates mechanical motion, sensor feedback, sound design, LED expression, and networked monitoring into a cohesive, interactive robotic platform.
