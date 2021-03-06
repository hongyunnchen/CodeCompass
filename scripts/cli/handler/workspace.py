import os
import termios
import shutil
import signal
import subprocess
import sys
import tarfile
import json
import copy

from os.path import join

from util.configfilehandler import ConfigFileHandler
from util import util

def __list_workspace(config_file_handler):
  '''Lists the available workspaces.'''
  print '\n'.join(config_file_handler.names())

def kill_child_processes(parent_pid, sig=signal.SIGTERM):
  ps_command = subprocess.Popen("ps -o pid --ppid %d --noheaders" % parent_pid, shell=True, stdout=subprocess.PIPE)
  ps_output = ps_command.stdout.read()
  retcode = ps_command.wait()
  assert retcode == 0, "ps command returned %d" % retcode
  for pid_str in ps_output.split("\n")[:-1]:
    os.kill(int(pid_str), sig)

def __delete_workspace(config_file_handler, args):
  '''Deletes a workspace (project dir, SQL database, CodeChecker runs)'''
  if not config_file_handler.exists(args.delete):
    sys.exit("Workspace '" + args.delete + "' does not exist!")

  #--- Remove from database ---#

  projectdir = join(args.workdir, args.delete)
  success = util.pgsql_cmd(projectdir, args, 'dropdb', args.delete)

  if (success != 0):
    sys.exit("Couldn't delete database " + args.delete + " because it doesn't "
    + "exist or bad connection (Host: " + args.dbhost + " Port: "
    + str(args.dbport) + " User: " + args.dbuser + ")")

  #--- Remove the project directory ---#

  if os.path.exists(projectdir):
    shutil.rmtree(projectdir)

  #--- Remove CodeChecker stuffs ---#

  runname = config_file_handler.get_attribute(
    args.delete, 'codecheckerrunname')

  if runname:
    try:
      checker_server, checker_port = util.start_codechecker_server(args)
    except Exception as e:
      sys.stderr.write('''CodeChecker database for the project could not be
        deleted, because CodeChecker was not found. Set CODECHECKER_HOME
        environment variable!\n''')
    else:
      command = ['CodeChecker', 'cmd', 'del']
      command.extend(['--host', args.dbhost])
      command.extend(['-p', str(checker_port)])
      command.extend(['-n', args.delete])

      cmd = 'source {}; {}'.format(
        join(os.environ['CODECHECKER_HOME'], 'init', 'init.sh'),
        ' '.join(command))

      proc = subprocess.Popen(
        ['/bin/bash', '-c', cmd],
        stdout = subprocess.PIPE,
        stderr = subprocess.PIPE,
        env = os.environ)
      proc.wait()
    finally:
      kill_child_processes(checker_server.pid)
      #os.killpg(os.getpgid(checker_server.pid), signal.SIGTERM) # it terminates console

  #--- Remove from config file ---#

  config_file_handler.remove(args.delete)
  config_file_handler.save()

def __rename_workspace(config_file_handler, args):
  '''Renames a workspace (project dir, SQL database). CodeChecker run name will
  not be modified.'''

  if not config_file_handler.exists(args.rename[0]):
    sys.exit("Workspace '" + args.rename[0] + "' does not exists!")
  if config_file_handler.exists(args.rename[1]):
    sys.exit("Workspace '" + args.rename[0] + "' already exists!")

  #--- Rename in config file ---#

  config_file_handler.rename(args.rename[0], args.rename[1])
  config_file_handler.save()

  #--- Rename the project directory ---#

  projectdir_from = join(args.workdir, args.rename[0])
  projectdir_to   = join(args.workdir, args.rename[1])

  if os.path.exists(projectdir_from):
    os.rename(projectdir_from, projectdir_to)

  #--- Rename database ---#

  sql = 'ALTER DATABASE "{}" RENAME TO "{}"'.format(
    args.rename[0], args.rename[1])

  util.pgsql_cmd(projectdir_to, args, 'psql', 'postgres', '-c', sql)

  #--- Rename paths in the database itself ---#

  sql = 'UPDATE "Option" SET value=REPLACE(value, \'{}\', \'{}\') WHERE ' \
    'key IN (\'projectDataDir\', \'searchIndexDir\')'.format(
    projectdir_from, projectdir_to)

  util.pgsql_cmd(projectdir_to, args, 'psql', args.rename[1], '-c', sql)

  sql = 'UPDATE "Option" SET value=\'{}\' WHERE key=\'name\''.format(args.rename[1])
  util.pgsql_cmd(projectdir_to, args, 'psql', args.rename[1], '-c', sql)

def __export_workspace(config_file_handler, args):
  '''Exports a workspace (project dir, SQL database)'''

  if not config_file_handler.exists(args.export[0]):
    sys.exit("Workspace '" + args.export[0] + "' does not exists!")

  #--- Dump database ---#
  projectdir = join(args.workdir, args.export[0])
  dump_filename = join(projectdir, 'database.dump')

  print "Dumping SQL database..."
  dumpRetCode = util.pgsql_cmd(
    projectdir,
    args,
    'pg_dump',
    '-f', dump_filename,
    '-F', 'tar',
    '-C', # Add CREATE DATABASE to the dump
    '-c', # Add DROP * to the dump
    '-O', # Do not set ownership in the dump
    '-b', # Include large objects in dump
    args.export[0])
  if dumpRetCode != 0 or not os.path.exists(dump_filename):
    sys.exit("Dumping the SQL database failed! Please check your database " \
             "connection options.")

  #--- Create CodeCompass.export ---#
  with open(join(projectdir, "CodeCompass.export"), 'w') as exp:
    json.dump({
      'name': str(args.export[0]),
      'config': config_file_handler.get_attributes(args.export[0])
    }, exp)

  #--- Create tar.gz file ---#
  try:
    print "Creating archive..."
    proc = subprocess.Popen(
      ['tar', '-C', args.workdir, '-czf', args.export[1], args.export[0]])
    proc.wait()
    print "Done"
  except Exception as e:
    sys.exit("Failed to create dump package!")
  finally:
    #--- Delete dumps ---#
    if os.path.exists(dump_filename):
      os.remove(dump_filename)

def __import_workspace(config_file_handler, args):
  '''Imports a workspace (project dir, SQL database)'''
  tgz_file = getattr(args, "import")
  if not os.path.exists(tgz_file):
    sys.exit(tgz_file + " does not exists!")

  #--- Get workspace name from tgz ---#
  with tarfile.open(tgz_file) as tar:
    files = tar.getmembers()
    if len(files) == 0:
      sys.exit(tgz_file + " is empty!")
    project_name = files[0].name
    projectdir = join(args.workdir, project_name)

  if config_file_handler.exists(project_name):
    sys.exit("Workspace '"+ project_name +"' already exists! Use --delete to " \
             "remove and then try again.")

  #--- Unpack tar.gz file ---#
  try:
    print "Unpacking project '" + project_name + "' from " + tgz_file
    proc = subprocess.Popen(
      ['tar', '-C', args.workdir, '-xzf', tgz_file])
    proc.wait()
  except Exception as e:
    sys.exit("Failed to unpack " + tgz_file)

  #--- Read CodeCompass.export ---#
  with open(join(projectdir, "CodeCompass.export"), 'r') as exp:
    exportData = json.load(exp)

  #--- Restore database ---#
  print "Restoring database"
  dump_filename = join(projectdir, 'database.dump')
  util.pgsql_cmd(projectdir, args, 'dropdb', project_name)
  restoreRetCode = util.pgsql_cmd(
    projectdir,
    args,
    'pg_restore',
    '-d', 'template1',
    '-F', 'tar',
    '-C', # Add CREATE DATABASE to the dump
    '-O', # Do not set ownership in the dump
    dump_filename)
  if restoreRetCode != 0:
    sys.exit("Failed! Please check " + join(projectdir, "dblog") + " for details.")
  else:
    #--- Delete dumps ---#
    if os.path.exists(dump_filename):
      os.remove(dump_filename)

  #--- Add to workspace ---#
  description = project_name;
  if 'config' in exportData and 'description' in exportData['config']:
    description = exportData['config']['description']

  connArgs = copy.copy(args)
  connArgs.name = project_name
  connection_string = util.create_connection_string(connArgs)

  config_file_handler.add(project_name)
  config_file_handler.set_attribute(project_name, 'connection', connection_string)
  config_file_handler.set_attribute(project_name, 'description', description)
  config_file_handler.save()
  print "Done"

def workspace_handler(args):
  config_file_handler = ConfigFileHandler(join(args.workdir, 'workspace.cfg'))

  if args.list:
    __list_workspace(config_file_handler)

  if args.delete:
    __delete_workspace(config_file_handler, args)

  if args.rename:
    __rename_workspace(config_file_handler, args)

  if args.export:
    __export_workspace(config_file_handler, args)

  if getattr(args, "import"):
    __import_workspace(config_file_handler, args)
