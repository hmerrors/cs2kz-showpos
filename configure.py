from ambuild2 import run
import os
parser = run.BuildParser(sourcePath=os.path.dirname(__file__), api='2.2')
parser.options.add_argument('--mms-path', required=True)
parser.options.add_argument('--sdk-path', required=True)
parser.options.add_argument('--tests', action='store_true')
parser.Configure()
