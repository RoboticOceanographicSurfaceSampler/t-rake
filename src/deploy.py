import sys

from filewatcher import Watcher
from temperature_rake import TemperatureRake

debug = False
debugdriver = False

def ExecuteDeploy(runstate):
  """ Callback sent to the file watcher that allows
      the deployment to run when a runfile is present.
  """
  print('Starting trake run with debug = ' + str(debug) + ' and debugdriver = ' + str(debugdriver))
  trake = TemperatureRake(runstate, debug, debugdriver)
  trake.Run()


"""
    Backend temperature rake deployment.
"""
if len(sys.argv) > 1:
  debug = True
  if sys.argv[1] == 'driver':
    debugdriver = True

watcher = Watcher(ExecuteDeploy, debug, debugdriver)
watcher.run()

exit(0)
