import os
import json
import time
import RPi.GPIO as GPIO
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler


configurationpath = '/trake/configuration'


class RunState:
    """ Carrier object that contains the state of the current
        run of the acquisition system.
    """
    def __init__(self):
        self.Reset()

    def Reset(self):
        self.configurationName = ''
        self.configuration = {}
        self.running = False
        self.runChange = False
        self.voltageLow = False

    def is_running(self):
        return self.running

    def is_voltage_low(self):
        return self.voltageLow
        
    def is_runchange(self):
        return self.runChange
    
    def get_configurationName(self):
        return self.configurationName
    
    def get_configuration(self):
        return self.configuration


class DeployHandler(FileSystemEventHandler):
    """ A filesystem event handler object customized to watch for
        the deploy file being created, deleted, or modified.
        The acquisition system is started when the deploy file
        is created or modified, and stopped when the deplpy file is deleted.
    """
    def __init__(self, debug):
        self.debug = debug
        self.runstate = RunState()

        # If the file exists at startup, run with it.
        deployFile = configurationpath + '/' + '__runfile__.deploy'
        if os.path.isfile(deployFile):
            self.handleNewOrModified(deployFile)

    def on_created(self, event):
        if self.debug:
            print('File created event: ' + event.src_path)
        self.handleNewOrModified(event.src_path)

    def on_modified(self, event):
        if event.is_directory:
            return
        
        if self.debug:
            print('File modified event: ' + event.src_path)
        self.handleNewOrModified(event.src_path)

    def on_deleted(self, event):
        self.handleDeleted(event.src_path)

    def handleNewOrModified(self, path):
        if path[-18:] == "__runfile__.deploy":
            if self.debug:
                print('Handling new runfile, reevaluating')
            self.reevaluate(path)
        elif path[-21:] == "__immediate__.execute":
            self.executeImmediate(path)

    def handleDeleted(self, path):
        if path[-18:] == "__runfile__.deploy":
            self.runstate.running = False

    def reevaluate(self, runFilePath):
        configurationName = ''
        configuration = {}
        running = False

        try:
            with open(runFilePath, 'r') as runfile:
                runData = json.load(runfile)
                if 'configurationName' in runData:
                    configName = runData['configurationName']
                    fullpathname = configurationpath + '/' + configName + ".json"
                    if os.path.isfile(fullpathname):
                        if self.debug:
                            print('Using configuration file ' + fullpathname)
                        with open(fullpathname, 'r') as configfile:
                            configurationName = configName
                            configuration = json.load(configfile)
                            running = True
                    else:
                        if self.debug:
                            print('Configuration file ' + fullpathname + ' does not exist')
        except Exception:
            pass

        runChange = False
        if self.runstate.configurationName != configurationName or self.runstate.running != running:
            runChange = True

        self.runstate.configurationName = configurationName
        self.runstate.configuration = configuration
        self.runstate.running = running
        self.runstate.runChange = runChange
        if self.debug:
            print('Run state: ' + self.runstate.configurationName + ' Running' if self.runstate.running else ' NOT Running' + ' runchange ' if self.runstate.runChange else ' NOT runchange ' + str(self.runstate.configuration))

    def executeImmediate(self, executeFilePath):
        # TODO - Execute the immediate command
        os.remove(executeFilePath)


POWER_LOW_Pin =  27     # Broadcom pin 27 (Pi pin 13)

class Watcher:

    def __init__(self, runHandler, debug):
        self.runHandler = runHandler
        self.debug = debug
        self.observer = Observer()
        self.handler = DeployHandler(self.debug)
        self.directory = configurationpath

    def run(self):
        self.observer.schedule(self.handler, self.directory, recursive=True)
        self.observer.start()
        if self.debug:
            print("\nWatcher Running in {}/\n".format(self.directory))
        #try:
        while not self.handler.runstate.is_voltage_low():
            time.sleep(0.1)
            if self.handler.runstate.is_running():
                if self.debug:
                    print('Runstate is running, calling run handler')
                self.runHandler(self.handler.runstate)
                self.handler.runstate.Reset()

            # Capture the low-voltage state.
            GPIO.setmode(GPIO.BCM)
            GPIO.setup(POWER_LOW_Pin, GPIO.IN)
            if not GPIO.input(POWER_LOW_Pin):
                if self.debug:
                    print('GPIO detected low voltage')
                self.handler.runstate.voltageLow = True
            GPIO.cleanup()

        #except:
        #    pass
        self.observer.stop()
        self.observer.join()
        if self.debug:
            print("\nWatcher Terminated\n")

