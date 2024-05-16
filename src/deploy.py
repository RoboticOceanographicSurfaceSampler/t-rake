import sys

from filewatcher import Watcher
from temperature_rake import TemperatureRake

debug = False

def ExecuteDeploy(runstate):
  """ Callback sent to the file watcher that allows
      the deployment to run when a runfile is present.
  """
  trake = TemperatureRake(runstate, debug)
  trake.Run()


"""
    Backend temperature rake deployment.
"""
if len(sys.argv) > 1:
  debug = True

watcher = Watcher(ExecuteDeploy)
watcher.run()

exit(0)
