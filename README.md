# Dryer_Monitor
 
This project will announce to your smart phone when your clothes dryer stops running. This is useful when your clothes dryer is in a location (such as a gargage) when you are unable to hear the buzzer alarm built into the dryer.

You will need to deploy a Particle Cloud webhook(s) found in the Other Files folder. One webhook gets alerts - the checked in version writes to a Slack instance to alert you that the dryer has turned on and then off. One webhook writes to Google sheet for debugging/information and should be turned off once you think the alert service is working well.

This project is currently running on a breadboard because it is so simple. We'll develop more documentation as time goes on. Contact us with any questions.
