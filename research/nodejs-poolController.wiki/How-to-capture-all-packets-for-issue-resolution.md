# Packet capture - 5.2 and later

Starting with 5.2, there is a new capture/replay tool available for troubleshooting.  This captures your logs, config file, incoming/outgoing packets, and URL actions.  The only thing you need to manually capture are any errors that are logged in the console (in the event of an app crash).

## How to capture all packets

1. Start the app with `npm run start:capture {config.json}`
1. After you experience the error/behavior/bug stop the app.  There will be three new files in the `replay` directory.  Include those three files when communicating any issues.

## What to include

1. What version of the code are you using
1. What is causing the error
1. All three files from the `/replay` directory.
1. Any errors that are in the console
1. Your pool equipment

# Packet capture - 5.1.x and earlier

In order to have a full picture of what is happening, and what is broken, it is necessary to see the full packet information that is output from the system.

## What to include

1. What version of the code are you using
1. What is causing the error
1. All output from the file log (See below)
1. Any errors that are in the console
1. Your `config.json` or in-use configuration file
1. Your pool equipment

### Setup your file log
In your `config.json`, there are a number of sections that you should enable:

```
        "log": {
            "logLevel": "info",
            "socketLogLevel": "info",
            "fileLog": {
                "enable": 1,   // Turn on the file log
                "fileLogLevel": "silly",  // leave this on 'silly' to capture everything
                "fileName": "output.log"
            },
            "logPumpMessages": 1,  // set all log variables to 1 to capture everything
            "logDuplicateMessages": 1,
            "logConsoleNotDecoded": 1,
            "logConfigMessages": 1,
            "logMessageDecoding": 1,
            "logChlorinator": 1,
            "logIntellichem": 1,
            "logPacketWrites": 1,
            "logPumpTimers": 1,
            "logReload": 01,
            "logApi": 1
```

##  Capture the error
Once you have the above setup, restart the app, wait for it to finish the configuration process (i.e. with Intellitouch) and then replicate the error.  Everything (except the error) should be captured in the logs.