import sys

from filewatcher import Watcher
from temperature_rake import TemperatureRake

debugdriver = False

def ExecuteDeploy(runstate):
  """ Callback sent to the file watcher that allows
      the deployment to run when a runfile is present.
  """
  trake = TemperatureRake(runstate, debugdriver)
  trake.Run()


"""
    Backend temperature rake deployment.
"""
debug = False
if len(sys.argv) > 1:
  debug = True
  if sys.argv[0] == 'driver':
    debugdriver = True

watcher = Watcher(ExecuteDeploy, debug, debugdriver)
watcher.run()

exit(0)
