# Integrations

Starting with 2.0, you can easily add integrations.  These would be middleware that would listen for output from the program and then do something with it.

## Listening

The listening part consists of Socket.IO events.  These events are:

1. Socket#circuit
1. Socket#time
1. Socket#temp
1. Socket#pump
1. Socket#heat
1. Socket#schedule
1. Socket#chlorinator
1. Socket#search
1. Socket#all

These are the same Socket.IO events that drive the bootstrap UI.

## Setup

1.  In 'config.json', add a new entry under "Integrations"  Our sample will be `outputToConsole`.

```
    "Integrations": {
        "outputToConsole": 1
    },
```

You can disable the integration by setting this value to 0.

1.  (optional) Add a section to `config.json` with any parameters that you want to pass to the integration

```
    "outputToConsole": {
      "level": "info"
    },
```

## JS File

Now that you are telling the app about the file, create a file with the same name in the `./integrations` directory.  This sample would be called `./integrations/outputToConsole.js`

### Contents of file

The file needs to have certain components included.  The skeleton of the file should be:
```
module.exports = function(container) {

    var io = container.socketClient
    var socket = io.connect('https://localhost:3000', {
        secure: true,
        reconnect: true,
        rejectUnauthorized: false
    });

    function init() {
        container.logger.verbose('outputToConsole Loaded')
    }

    return {
        init: init
    }
}
```

## Subscribing to Sockets

To listen to the events, simply call them like:
```
    //This is a listener for the time event.  data is what is received.
    socket.on('time', function(data){
      console.log('The time was broadcast, and outputToConsole received it.  The time is: %s', JSON.stringify(data))
    })
```

`data` is a JSON object and can be parsed through.  It varies depending upon the event that is listening.

# Included integrations

## outputToSocketExample
A sample that shows how to connect with the base code.
    
Add the following to your `config.json` at the same level as the poolController.

    ```
    "integrations": {
        "outputSocketToConsoleExample": 1
    },
    "outputSocketToConsoleExample": {
        "level": "warn"
    }
    ```

## socketISY
An integration to connect the poolController app to your ISY hub.

Add the following to your `config.json` at the same level as the poolController.

```
 "socketISY": {
        "username": "blank",
        "password": "blank",
        "ipaddr": "127.0.0.1",
        "port": 12345,
        "Variables": {
            "chlorinator": {
                "saltPPM": 16
            },
            "pump": {
                "1": {
                    "watts": 25,
                    "rpm": 24,
                    "currentprogram": 13,
                    "program1rpm": 10,
                    "program2rpm": 11,
                    "program3rpm": 12,
                    "program4rpm": 13,
                    "power": 14,
                    "timer": 15
                }
            },
            "circuit": {
                "1": {
                    "status": 8
                },
                "2": {
                    "status": 3
                },
                "3": {
                    "status": 2
                }
            },
            "temperatures": {
                "poolTemp": 17,
                "spaTemp": 18,
                "airTemp": 19,
                "spaSetPoint": 20
            }
        }
    }
```
