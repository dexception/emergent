#!/usr/bin/env python
import datetime, os, re, subprocess, sys, time, traceback, ConfigParser, socket
import xml.etree.ElementTree as ET

#############################################################################
# STANDARD USER CONFIGURABLE PARAMETERS

# full path to single processor job submission script
# STRONGLY recommend using the pyqsub based commands avail in 
# emergent/cluster_run/ directory (where this script lives as well)
#
# the sp_qsub_cmd takes args of <n_threads> <run_time> <full_command>
sp_qsub_cmd = '/usr/local/bin/sp_qsub_q'

# the dm_qsub_cmd takes args of <mpi_nodes> <n_threads> <run_time> <full_command>
dm_qsub_cmd = '/usr/local/bin/dm_qsub_q'

# it is essential that these scripts return the cluster job number in the format
# created: JOB.<jobid>.sh -- we parse that return val to get the jobid to monitor
# further (you can of course do this in some other way by editing code below)

# qstat-like command -- for quering a specific job_no 
# sge = qstat -j <job_no>
# moab = checkjob is better, but otherwise qstat <job_no>
# job_no will automatically be appended to end of command
qstat_cmd = "qstat"
qstat_args = "-j"  # here is where you put the -j if needed

# regexp for output of qstat that tells you that the job is running
qstat_running_re = r"^usage"
# use this for "split" command to get misc running info
qstat_running_info_split = "usage"
# regexp for output of qstat that tells you that the job is still queued
qstat_queued_re = r"^scheduling info:"
# use this for "split" command to get misc queued info
qstat_queued_info_split = "scheduling info:"

# qdel-like command -- for killing a job
# killjob is a special command that also deletes the JOB.* files -- see pykilljob
# be sure to use the _f version that does not prompt!
# in emergent/cluster_run directory
# job_no will automatically be appended to end of command
qdel_cmd = "killjob_f"
qdel_args = ""

# number of runtime minutes during which the script will continue to update the 
# output info from the job (job_out, dat_files)
job_update_window = 3

# set to true for more debugging info
debug = False

# END OF STANDARD USER CONFIGURABLE PARAMETERS
#############################################################################

# NOTE: this is not currently used:
# showq-like command -- this should return overall status of all users jobs
# user name will be appended to end of command
# moab = showq -w user=<username>
# pyshowq for SGE (checked into emergent/cluster_run showq <username>
# showq_cmd = "/usr/local/bin/showq"
# showq_args = ""  # for moab, use '-w user='

# this is NOT user configurable but is a global setting that should agree with 
# how time is formatted in emergent
time_format = "%Y_%m_%d_%H_%M_%S"


# status docs:
# The client sets this field in the jobs_submit table to:
#   REQUESTED to request the job be submitted.
#   CANCELLED to request the job indicated by job_no or tag be cancelled.
#   PROBE     probe to get the cluster to track this project, and update all running
#   GETDATA   get the data for the associated tag -- causes cluster to check in dat_files
#   GETFILES  tell cluster to check in all files listed in this other_files entry
# The cluster script sets this field in the running/done tables to:
#   SUBMITTED after job successfully submitted to a queue.
#   QUEUED    when the job is known to be in the cluster queue.
#             At this point, we have a job number (job_no).
#   RUNNING   when the job has begun.
#   DONE      if the job completed successfully.
#   KILLED    if the job was cancelled.


def make_dir(dir):
    try: os.makedirs(dir)
    except os.error: pass

# Don't have subprocess.check_output in Python 2.6.  This will have to do.
def check_output(cmd):
    return subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()[0]

#############################################################################

class ClusterConfig(object):
    def __init__(self, wc_root):
        # Ensure the working copy root directory exists.
        self.wc_root = wc_root
        make_dir(self.wc_root)

        # Read cached configuration data from an .ini file, if present.
        self.config_filename = os.path.join(self.wc_root, 'config.ini')
        self.config = ConfigParser.RawConfigParser()
        if os.path.isfile(self.config_filename):
            self.config.read(self.config_filename)

        # Ensure all needed sections are present (e.g., first script run).
        self.repo_section = 'repo_names'
        self.user_section = 'user_data'
        self._ensure_section_exists(self.repo_section)
        self._ensure_section_exists(self.user_section)

    def _ensure_section_exists(self, section):
        if not self.config.has_section(section):
            self.config.add_section(section)

    def _write_config(self):
        # Write any changes (username, new repo) to the config file.
        with open(self.config_filename, 'wb') as f:
            self.config.write(f)

    def _prompt_for_field(self, section, field, message, default=""):
        # Get the cached value (if any) otherwise use the default.
        try:    value = self.config.get(section, field)
        except: value = default

        # Allow the user to choose a different value.
        prompt = message + ' '
        if value: prompt += '[%s] ' % value
        other_value = raw_input(prompt)
        if other_value: value = other_value

        # Cache the value in the config object and return it.
        self.config.set(section, field, value)
        self._write_config()
        return value

    def _prompt_for_int_field(self, section, field, message, default=""):
        while True:
            try:
                return int(self._prompt_for_field(
                    section, field, message, default))
            except ValueError: pass

    def get_username(self):
        # Get the user's Subversion username (defaults to cached value or
        # $USER).
        return self._prompt_for_field(
            self.user_section, 'username',
            'Enter your Subversion username:', os.environ['USER'])

    def get_clustername(self):
        # Get the name of this cluster.
        host = socket.gethostname()
        if host.find('.'):
            host = host.split('.')[0]
        return self._prompt_for_field(
            self.user_section, 'clustername',
            'Enter the name of this cluster:', host)

    def get_poll_interval(self):
        # Get the amount of time between polling the subversion server.
        return self._prompt_for_int_field(
            self.user_section, 'poll_interval',
            'Enter the polling interval, in seconds (10-20 recommended):')

    def get_check_user(self):
        # Should the script only start jobs committed by the selected user?
        check = self._prompt_for_field(
            self.user_section, 'check_user',
            'Only run jobs you committed?', 'Yes')
        return check and check[0] in 'yY'

    def get_repo_choice(self):
        return self._prompt_for_int_field(
            self.user_section, 'repo_choice', 'Your choice:')

    def choose_repo(self):
        # Get the list of repos to choose from.
        repos = sorted(self.config.items(self.repo_section))
        if repos:
            print '\nChoose a repository to monitor:'
            print '  0. New repo...'
            for i, (name, url) in enumerate(repos, start=1):
                print '  %d. %s = %s' % (i, name, url)

            print ''
            while True:
                choice = self.get_repo_choice()
                if choice <= i: break

            if choice > 0:
                return repos[int(choice) - 1]

        repo_name = raw_input('\nEnter name of new repository: ')
        repo_url  = raw_input(  'Enter URL of new repository:  ')
        self.config.set(self.repo_section, repo_name, repo_url)
        self._write_config()
        return (repo_name, repo_url)

#############################################################################
    
class DataTable(object):
    
    # NOTE: new types added to this class must also be added to the following methods of the DataTable class:
    # encode_value, decode_value, get_typed_val
    class ColumnType(object):
    
        STRING = 'str'
        INT = 'int'
        FLOAT = 'float'
        
        STRING_CODE = '$'
        INT_CODE = '|'
        FLOAT_CODE = '%'
        
        encode = {STRING : STRING_CODE,
                  INT : INT_CODE,
                  FLOAT: FLOAT_CODE}
        
        decode = {STRING_CODE : STRING,
                  INT_CODE : INT,
                  FLOAT_CODE : FLOAT}       

    DELIMITER = '\t'  
    
    escape_map = [('\\', '\\\\'),     # \ --> \\  (must be first)
                  ("'",  "\\'"),      # ' --> \'
                  ('"',  '\\"'),      # " --> \"
                  ('\t', ' '),        # (tab) --> (space)
                  ('\n', '\\n')]      # 
        
    def __init__(self):
        self._header = [] # header of the data table 
        self._rows = []  # data of the data table
    
    #input: space_allowed = boolean, if False replace white spaces with '_'
    @staticmethod
    def escape_chars(string, space_allowed=True):
        for char, replacement in DataTable.escape_map:
            string = string.replace(char, replacement)
        if not space_allowed:
            string = string.replace(' ', '_') # (space) --> _  (must be last)
        return string    
     
    #input: space_allowed = boolean, if False replace white spaces with '_'
    @staticmethod
    def unescape_chars(string):
        for char, replacement in DataTable.escape_map:
            string = string.replace(replacement, char)
        return string    
     
    # return a typed value for a value
    # input: v = value to be turned into a typed value
    #        t = the destination type
    @staticmethod
    def get_typed_val(v, t):
        try:
            if t == DataTable.ColumnType.STRING: return v
            if t == DataTable.ColumnType.INT: return int(v)
            if t == DataTable.ColumnType.FLOAT: return float(v)
        except:
            print 'Value [%s] and type %s mismatch.' % (v, t)
            return False
    
    # PRIVATE METHODS
    @staticmethod
    def encode_val(val, col_type):
        encoded_val = ''
        if col_type is DataTable.ColumnType.STRING:
            encoded_val = '"%s"' % DataTable.escape_chars(val)
        elif col_type is DataTable.ColumnType.INT:
            encoded_val = val
        elif col_type is DataTable.ColumnType.FLOAT:
            encoded_val = val
        else:
            print "Column type '%s' is not supported and cannot be encoded." % col_type
            return False
            
        return encoded_val
    
    @staticmethod
    def decode_val(val, col_type):
        decoded_val = ''
        if col_type is DataTable.ColumnType.STRING:
            decoded_val = DataTable.unescape_chars(val.lstrip('"').rstrip('"'))
        elif col_type is DataTable.ColumnType.INT:
            decoded_val = val
        elif col_type is DataTable.ColumnType.FLOAT:
            decoded_val = val
        else:
            print "Column type '%s' is not supported and cannot be decoded." % col_type
            return False                    
            
        return decoded_val
            
    # takes the first line of a .dat file and sets up self._header
    # input: header_str = string, the first row of a .dat file    
    def _load_header(self, header_str):
        s = '_H:%s' % self.DELIMITER
        header_str = header_str.lstrip(s).rstrip('\n')
        cols = header_str.split(self.DELIMITER)
        
        header = [ {'name': c[1:], 'type':  DataTable.ColumnType.decode[c[0]]} for c in cols ]
        self.set_header(header)
            
    # takes a list of rows from a .dat file and sets self.rows
    # input: rows = list, list of rows containing the data part of a .dat file
    def _load_data(self, rows):
        s = '_D:%s' % self.DELIMITER
        for r in rows:
            r = r.lstrip(s).rstrip('\n').strip('\t')
            vals = r.split(self.DELIMITER)
            decoded_vals = []
            #col_idx = 0
            for i, v in enumerate(vals):
                #col_type = self._header.vals()[col_idx]
                col_type = self._header[i]['type']
                decoded_vals.append(DataTable.decode_val(v, col_type))
                #col_idx += 1
            self.set_row(decoded_vals)                

    # PUBLIC METHODS
    
    # input: header(optional) = a list, the header of data table with this format [{'name': col name, 'type': col type}, ...]
    # output: the header as string
    def get_header_str(self, header=None):
        if header is None:
            header = self._header
        header_str = "_H:"
        for i in range(len(header)):
            col_type = DataTable.ColumnType.encode[self.get_col_type(i)]
            col_name = DataTable.escape_chars(self.get_col_name(i), space_allowed=False)
            header_str += '%s%s%s' % (self.DELIMITER, col_type, col_name)
            
        return header_str
    
    # returns the header of the data table
    # output: list, the header of data table with this format [{'name': col name, 'type': col type}, ...]
    def get_header(self):
        return self._header

    def reset_data(self):
        self._rows = []  # reset
        
    # copy columns from another table -- resets row data too!
    def copy_cols(self, other_table):
        self._header = other_table._header
        self.reset_data()
    
    # returns a row as string
    # input: row_idx = int; the index of the row
    # output: the row as string
    def get_row_str(self, row_idx):
        row_str = '_D:'
        for col_idx, val in enumerate(self._rows[row_idx]):
            col_type = self.get_col_type(col_idx)
            val = DataTable.encode_val(val, col_type)
            row_str += '%s%s' % (self.DELIMITER, val)
            
        return row_str

    # number of rows
    def get_n_rows(self):
        return len(self._rows)
    
    # number of cols
    def get_n_cols(self):
        return len(self._header)
    
    # returns a row of the data table given its row index
    def get_row(self, row_num):
        try:
            return self._rows[row_num]
        except:
            return False

    def remove_row(self, row_num):
        try:
            del self._rows[row_num]
        except:
            print "remove_row: Row number %d doesn't exist" % row_num
    
    # returns the data section (rows) of the data table
    # output: list, list of rows
    def get_data(self):
        return self._rows
    
    # returns the type of a column given its index
    def get_col_type(self, idx):
        try:
            return self._header[idx]['type']
        except:
            print "Column index %s is out of range." % idx
            return False
    
    # returns the name of a column given its index
    def get_col_name(self, idx):
        try:
            return self._header[idx]['name']
        except:
            print "Column index %s is out of range." % idx
            return False
    
    # returns the index of a column given its name  
    def get_col_idx(self, col_name):
        for i in range(len(self._header)):
            if self.get_col_name(i) == col_name:
                return i
        print "Column '%s' doesn't exist." % col_name
        return False

    # adds a new column to the data table in memory
    def add_col(self, col_name, col_type):
        if self.get_col_idx(col_name):
            self._header.append({'name': col_name, 'type': col_type})
            for r in self._rows:    # add an empty columns to the data rows
                r.append('')
            return True
        else:
            print "Column '%s' (%s) already exists." % (col_name, col_type)
            return False   
    
    # validates a value with regard to a column
    # input: val = value to validate
    #        col_name = the column name of the value
    def validate_val(self, val, col_name):
        col_idx = self.get_col_idx(col_name)
        col_type = self.get_col_type(col_idx)
        return True if self.get_typed_val(val, col_type) else False

    # returns the value of a single cell
    # input: row_num = the row index of the cell to update
    #        col_name = the column name of the cell to update
    def get_val(self, row_num, col_name):
        col_idx = self.get_col_idx(col_name)
        try:
            row = self._rows[row_num]
            str_val = row[col_idx]
        except:
            print "No cell found under column '%s' at row number %s." % (col_name, row_num)
            return False
        
        return self.get_typed_val(str_val, self.get_col_type(col_idx))
        
    # sets value of a single cell
    # input: row_num = the row index of the cell to update
    #        col_name = the column of the cell to update
    #        val = the new value of the cell 
    def set_val(self, row_num, col_name, val):
        col_idx = self.get_col_idx(col_name)
        if col_idx >= 0: #and self.get_typed_val(val, self.get_col_type(col_idx)):
            try:
                self._rows[row_num][col_idx] = str(val)
                return True
            except:
                print "Row number %s doesn't exist, col_idx: %s." % (row_num, col_idx)
                return False
        else:
            print "Col named %s doesn't exist, col_idx: %s." % (col_name, col_idx)
            return False
        
    # find the (first) index of item incol name (-1 if not found)
    # input: col_name = the column name of the cell to update
    #        val = value to find
    def find_val(self, col_name, val):
        col_idx = self.get_col_idx(col_name)
        if col_idx >= 0: 
            for i in range(len(self._rows)):
                str_val = self._rows[i][col_idx]
                cval = self.get_typed_val(str_val, self.get_col_type(col_idx))
                if cval == val:
                    return i
            return -1    # not found
        else:
            print "Col named %s doesn't exist, col_idx: %s." % (col_name, col_idx)
        return -1

    def set_header(self, header):
        self._header = header
    
    # sets values of a [new] row      
    # input: vals = list, list of values in the row
    #        row_num = int, the index of row to update            
    def set_row(self, vals, row_num=None):
        if len(vals) is not len(self._header):
            return False
        if row_num is not None:
            self._rows[row_num] = vals
        else:    
            self._rows.append(vals)
        return True

    # append row from another table
    def append_row_from(self, other, other_row):
        vals = other.get_row(other_row)
        self.set_row(vals)
    
    # loads a .dat file into memoory
    # input: path = string, path to the .dat file
    def load_from_file(self, path):
        self.reset_data()
        with open(path, 'r') as f:
            lines = f.readlines()
            header = lines[0]
        rows = lines[1:]
        self._load_header(header)
        self._load_data(rows)
        
    # writes the data table into a .dat file
    # input: path = string, path of the destination .dat file
    #        append(optional) = boolean, if True append, otherwise overwrite
    def write(self, path, append=False):
        # mode = 'a' append, mode = 'w' overwrite 
        mode = 'a' if append else 'w'
        with open(path, mode) as f:
            if len(self._header) and not append:
                f.write(self.get_header_str() + '\n')
            for i in range(len(self._rows)):
                f.write(self.get_row_str(i) + '\n')

#############################################################################

class SubversionPoller(object):
    def __init__(self, username, repo_dir, repo_url, delay, check_user):
        self.username   = username
        self.repo_dir   = repo_dir
        self.repo_url   = repo_url
        self.delay      = delay
        self.check_user = check_user

        self.submit_files = ""
        self.model_files = ""
        self.all_running_files = set()
        self.cur_proj_root = ""  # current project root directory (parent of subdirs)
        self.cur_submit_file = ""
        self.cur_running_file = ""
        self.cur_done_file = ""
        self.jobs_submit = DataTable()
        self.jobs_running = DataTable()
        self.jobs_done = DataTable()
        self.file_list = DataTable()

        # The repo directory only includes the working copy root and
        # repo_name (as defined by the user and stored in config.ini).
        # It does not include the project name, hence the '/[^/]+/'.
        # The RE captures the whole (relative) filename to be retrieved
        # later in a m.group(1) call.
        esc_repo_dir = re.escape(self.repo_dir)
        self.sub_re_comp = re.compile(
            r'^\s*[AUGR]\s+(%s/[^/]+/submit/.*)' % esc_repo_dir)
        self.mod_re_comp = re.compile(
            r'^\s*[AUGR]\s+(%s/[^/]+/models/.*)' % esc_repo_dir)

    def get_initial_wc(self):
        # Either checkout or update the directory.
        if os.path.isdir(self.repo_dir):
            cmd = ['svn', 'up', '--username', self.username, '--force',
                   '--accept', 'theirs-full', self.repo_dir]
        else:
            cmd = ['svn', 'co', '--username', self.username,
                   self.repo_url, self.repo_dir]

        # Run command and make sure the directory exists afterwards.
        # This initial checkout/update is interactive so the user can
        # enter credentials if needed, which will then be cached for
        # the remainder of this script.
        subprocess.call(cmd)
        if not os.path.isdir(self.repo_dir):
            print '\nCheckout failed, aborting.'
            sys.exit(1)

        # Now do an 'svn info' (non-interactive) to get the starting rev.
        revision, author = self._get_commit_info(self.repo_dir)
        return revision

    # this is the main loop!
    def poll(self, nohup_file=''):
        # Enter the loop to check for updates to job submission files
        # and to query the job scheduler regarding submitted jobs.
        print '\nPolling the Subversion server every %d seconds ' \
              '(hit Ctrl-C to quit) ...' % self.delay
        while True:
            # If running in background and the nohup "keep running" file
            # was deleted, then exit.
            if nohup_file and not os.path.isfile(nohup_file):
                break

            sys.stdout.write('.') # TODO: remove, or create spinner...?
            sys.stdout.flush()

            # Update the working copy and get the list of files updated in
            # the 'submit'-folder (typically only one file at a time).
            sub_files = self._check_for_updates()

            # Things to do each poll cycle, in order:
            # 1. Cancel existing jobs and submit new jobs as requested in
            #    any submission files that were committed.  Those files
            #    will be added to the self.all_submit_files set.
            for filename in sub_files:
                if debug:
                    print '\nProcessing %s' % filename
                self._process_new_submission(filename)

            # Remaining steps are done on *all* running files seen so far.
            for filename in self.all_running_files:
                # 2. Query the status of submitted jobs.  Any cancellations
                #    or new submissions have already been made, so we won't
                #    query cancelled jobs (or when we do, their status will
                #    be "KILLED") and so we get (possibly) updated status
                #    of jobs just submitted.
                self._query_submitted_jobs(filename)

            # 3. Commit the changes.
            self._commit_changes()

            # 4. Sleep until next poll cycle.
            time.sleep(self.delay)

    def _check_for_updates(self):
        cmd = ['svn', 'up', '--username', self.username, '--force',
               '--accept', 'theirs-full', '--non-interactive', self.repo_dir]
        svn_update = check_output(cmd)

        # print svn_update

        # 'svn up' does not allow an '--xml' parameter, so we need to
        # scrape the textual output it produces.
        # The 'm' MatchObject iterates over a list of length 1.
        self.submit_files = [m.group(1) for l in svn_update.splitlines()
                                   for m in [self.sub_re_comp.match(l)] if m]
        self.model_files = [m.group(1) for l in svn_update.splitlines()
                                   for m in [self.mod_re_comp.match(l)] if m]
        return self.submit_files

    def _process_new_submission(self, filename):
        try:
            # Check if the commit author matches (if requested).
            rev, author = self._get_commit_info(filename)
            if self.check_user and author != self.username:
                print 'Ignoring job submitted by user %s:' % author
                print '  File: %s' % filename
            else:
                # Start jobs for any 'jobs_submit.dat' files.
                if os.path.basename(filename) == 'jobs_submit.dat':
                    self._get_cur_jobs_files(filename)
                    self._start_or_cancel_jobs(filename, rev)
                else:
                    self._unexpected_file(filename, rev)
        except:
            traceback.print_exc()
            print '\nCaught exception trying to parse job ' \
                  'submission file'
            print 'Continuing to poll the Subversion server ' \
                  '(hit Ctrl-C to quit) ...'

    def _query_submitted_jobs(self, filename, force_updt = False):
        self._get_cur_jobs_files(filename) # get all the file names for this dir
        # Load running into current
        self.jobs_running.load_from_file(filename)
        self.cur_running_file = filename

        # go backward because we can delete from running.. also newest to oldest
        for row in reversed(range(self.jobs_running.get_n_rows())):
            status = self.jobs_running.get_val(row, "status")
            tag = self.jobs_running.get_val(row, "tag")
            # print "updating status of job tag: %s status: %s" % (tag, status)

            if status == 'SUBMITTED' or status == 'QUEUED':
                self._query_job_queue(row, status)
            elif status == 'RUNNING':
                self._query_job_queue(row, status)   # always update status!
                self._update_running_job(row, force_updt)
            elif status == 'CANCELLED':
                self._query_job_queue(row, status)   # always update status!
                status = self.jobs_running.get_val(row, "status") # check right away
                if status == 'KILLED':
                    self._move_job_to_done(row)
            elif status == 'DONE' or status == 'KILLED':
                self._move_job_to_done(row)

        # write the new jobs running status
        self.jobs_running.write(self.cur_running_file)

    # setup current table filenames based on any given filename in status
    def _get_cur_jobs_files(self, filename):
        self.cur_proj_root = os.path.dirname(filename)
        self.cur_proj_root = os.path.dirname(self.cur_proj_root)
        self.cur_submit_file = self.cur_proj_root + '/submit/jobs_submit.dat'
        self.cur_running_file = self.cur_proj_root + '/submit/jobs_running.dat'
        self.cur_done_file = self.cur_proj_root + '/submit/jobs_done.dat'
 
    def _get_commit_info(self, filename):
        """Get the commit revision and author for the given filename."""

        cmd = ['svn', 'info', '--xml', filename]
        svn_info = check_output(cmd)

        root = ET.fromstring(svn_info)
        try:    revision = root.getiterator('commit')[0].attrib['revision']
        except: revision = '0'

        try:    author = root.getiterator('author')[0].text
        except: author = ''

        return (revision, author)

    # initialize the jobs_running table -- either load from file or init from submit
    def _init_jobs_running(self):
        if self.jobs_running.get_n_cols() == 0:
            if os.path.exists(self.cur_running_file):
                if debug:
                    print "loading previous running file: %s" % self.cur_running_file
                self.jobs_running.load_from_file(self.cur_running_file)
            else:
                self.jobs_running.copy_cols(self.jobs_submit)
        
    # initialize the jobs_done table -- either load from file or init from running
    def _init_jobs_done(self):
        if self.jobs_done.get_n_cols() == 0:
            if os.path.exists(self.cur_done_file):
                if debug:
                    print "loading previous done file: %s" % self.cur_done_file
                self.jobs_done.load_from_file(self.cur_done_file)
            else:
                self.jobs_done.copy_cols(self.jobs_running)

    def _start_or_cancel_jobs(self, filename, rev):
        # Load the new 'submit' table from the working copy.
        self.jobs_submit.load_from_file(filename)

        for row in range(self.jobs_submit.get_n_rows()):
            status = self.jobs_submit.get_val(row, "status")

            if status == 'REQUESTED':
                self._start_job(filename, rev, row)
                # write the new jobs running status
                self.jobs_running.write(self.cur_running_file)
                self._svn_add_cur_running()   # could be first time -- checkin if necc
            elif status == 'CANCELLED':
                self._cancel_job(filename, rev, row)
                self.jobs_running.write(self.cur_running_file)
                self._add_cur_running_to_list()  # monitor us going forward
            elif status == 'GETDATA':
                self._getdata_job(filename, rev, row)
                self._add_cur_running_to_list() # monitor us going forward
            elif status == 'GETFILES':
                self._getfiles_job(filename, rev, row)
                self._add_cur_running_to_list() # monitor us going forward
            elif status == 'PROBE':
                self._add_cur_running_to_list()  # monitor us going forward
                self._query_submitted_jobs(self.cur_running_file, True)  # True = force updt
                if debug:
                    print "probed for project root %s" % self.cur_proj_root

    def _start_job(self, filename, rev, row):
        if len(self.model_files) != 1:   # no project committed!
            print "\nNo project file was submitted along with job submit commit -- unable to run"
            return

        proj = self.model_files[0]  
        proj = os.path.abspath(proj)

        cmd = self.jobs_submit.get_val(row, "command")
        params = self.jobs_submit.get_val(row, "params")
        run_time = self.jobs_submit.get_val(row, "run_time")
        n_threads = self.jobs_submit.get_val(row, "n_threads")
        mpi_nodes = self.jobs_submit.get_val(row, "mpi_nodes")
        submit_svn = str(rev)
        submit_job = str(row)
        tag = '_'.join((submit_svn, submit_job))

        cmd = cmd.replace("<TAG>", "_" + tag)  # leading ubar added here only
        cmd = cmd.replace("<PROJ_FILENAME>", proj)

        cmdsub = []
        if mpi_nodes <= 1:
            cmdsub = [sp_qsub_cmd, str(n_threads), run_time, cmd, params]
        else:
            cmdsub = [dm_qsub_cmd, str(mpi_nodes), str(n_threads), run_time, cmd, params]
        # print 'command: %s' % cmdsub

        result = check_output(cmdsub)
        # print "result: %s" % result

        re_job = re.compile('created: JOB')

        job_no = ''
        for l in result.splitlines():
            if re_job.match(l):
                job_no = l.split('JOB.')[1].split('.sh')[0]

        # print "job_no: %s" % job_no
        job_out_file = "JOB." + job_no + ".out"

        status = 'SUBMITTED'

        # get our jobs running table all setup
        self._init_jobs_running()

        # grab current submit info
        self.jobs_running.append_row_from(self.jobs_submit, row)
        run_row = self.jobs_running.get_n_rows()-1
        self.jobs_running.set_val(run_row, "submit_svn", submit_svn)
        self.jobs_running.set_val(run_row, "submit_job", submit_job)
        self.jobs_running.set_val(run_row, "job_no", job_no)
        self.jobs_running.set_val(run_row, "command", cmd)
        self.jobs_running.set_val(run_row, "params", params)
        self.jobs_running.set_val(run_row, "tag", tag)
        self.jobs_running.set_val(run_row, "status", status)
        self.jobs_running.set_val(run_row, "job_out_file", job_out_file)

    def _cancel_job(self, filename, rev, row):
        job_no = self.jobs_submit.get_val(row, "job_no")
        if qdel_args != '':
            cmd = [qdel_cmd, qdel_args, job_no]
        else:
            cmd = [qdel_cmd, job_no]
        del_out = check_output(cmd)
        if debug:
            print "killed job: %s output: %s" % (job_no, del_out)
        # update jobs running so that job is correctly marked at KILLED instead of DONE
        self._init_jobs_running()
        runrow = self.jobs_running.find_val("job_no", job_no)
        if runrow >= 0:
            status = 'CANCELLED'
            self.jobs_running.set_val(row, "status", status)
        # we don't actually do anything with this output..


    def _getdata_job(self, filename, rev, row):
        tag = self.jobs_submit.get_val(row, "tag")
        self._init_jobs_running()  # make sure we have the running files.. this is a submit path
        runrow = self.jobs_running.find_val("tag", tag)
        if runrow >= 0:
            dat_files = self.jobs_running.get_val(runrow, "dat_files")
            self._svn_add_dat_files(tag, dat_files)
        else:
            self._init_jobs_done() # make sure we have the done files loaded
            donerow = self.jobs_done.find_val("tag", tag)
            if donerow >= 0:
                dat_files = self.jobs_done.get_val(donerow, "dat_files")
                self._svn_add_dat_files(tag, dat_files)
            else:
                print "getdata_job: tag %s not found in either jobs running or done" % tag

    def _getfiles_job(self, filename, rev, row):
        other_files = self.jobs_submit.get_val(row, "other_files")
        self._svn_add_dat_files("no tag", other_files)

    def _svn_add_dat_files(self, tag, dat_files):
        if len(dat_files) == 0:
            if debug:
                print "tag: %s has no dat files (yet)" % tag
            return
        dats = dat_files.split(' ')
        resdir = self.cur_proj_root + "/results/"
        for df in dats:
            fdf = resdir + df
            # print "dat file: %s" % fdf
            if os.path.exists(fdf):
                cmd = ['svn', 'add', '--username', self.username,
                       '--non-interactive', fdf]
                # Don't check_output, just dump it to stdout (or nohup.out).
                subprocess.call(cmd)

    def _unexpected_file(self, filename, rev):
        print 'Ignoring file committed to "submit" folder ' \
              'in revision %s: %s' % (rev, filename)

    def _job_is_done(self, row, status):
        tag = self.jobs_running.get_val(row, "tag")
        stdt = datetime.datetime.now()
        end_time = stdt.strftime(time_format)
        self.jobs_running.set_val(row, "status", status)
        self.jobs_running.set_val(row, "end_time", end_time)
        if debug:
            print "job: %s ended with status: %s at time: %s" % (tag, status, end_time)

    def _query_job_queue(self, row, status):
        job_no = self.jobs_running.get_val(row, "job_no")
        tag = self.jobs_running.get_val(row, "tag")
        if qstat_args != '':
            cmd = [qstat_cmd, qstat_args, job_no]
        else:
            cmd = [qstat_cmd, job_no]
        q_out = check_output(cmd)

        re_running = re.compile(qstat_running_re)
        re_queued = re.compile(qstat_queued_re)

        got_status = False

        # todo: trap output for when scheduler just isn't working properly
        # need to be robust to this and not mark a job as killed..

        for l in q_out.splitlines():
            if re_running.match(l):
                if status != 'RUNNING' and status != 'CANCELLED':
                    status = 'RUNNING'
                    stdt = datetime.datetime.now()
                    start_time = stdt.strftime(time_format)
                    self.jobs_running.set_val(row, "status", status)
                    self.jobs_running.set_val(row, "start_time", start_time)
                    # print "qstat job status update -- tag: %s now: %s start: %s info: %s" % (tag, status, start_time, status_info)
                status_info = l.split(qstat_running_info_split)[1]
                status_info = status_info.strip(' \t\n\r')
                self.jobs_running.set_val(row, "status_info", status_info)
                got_status = True
                break
            if re_queued.match(l):
                # cannot go from running or cancelled back to queued.. 
                if status != 'RUNNING' and status != 'CANCELLED':
                    status = 'QUEUED'
                    status_info = l.split(qstat_queued_info_split)[1]
                    status_info = status_info.strip(' \t\n\r')
                    self.jobs_running.set_val(row, "status", status)
                    self.jobs_running.set_val(row, "status_info", status_info)
                    # print "qstat job status update -- tag: %s now: %s info: %s" % (tag, status, status_info)
                got_status = True
                break
        if not got_status:
            if status == 'SUBMITTED' or status == 'REQUESTED' or status == 'DONE':
                pass    # don't do anything
            elif status == 'RUNNING':
                self._job_is_done(row, 'DONE')
            elif status == 'CANCELLED':
                self._job_is_done(row, 'KILLED')
            elif status == 'QUEUED':
                self._job_is_done(row, 'KILLED')
        # done

    def _get_job_out(self, job_out_file):
        job_out = ''
        if os.path.exists(job_out_file):
            with open(job_out_file, 'r') as f:
                lines = f.readlines()
            job_out = ''.join(lines[:12])  # just get start of file
        if len(job_out) > 1024:
            job_out = job_out[0:1024]
        # print "from out file: %s got job out: %s" % (job_out_file, job_out)
        return job_out

    def _get_dat_files(self, tag):
        # next look for output files
        results_dir = self.cur_proj_root + "/results/"

        re_tag_dat = re.compile(r".*%s.*\.dat" % tag)
        re_tag = re.compile(r".*%s.*" % tag)

        dat_files_set = set()
        other_files_set = set()
        resfiles = os.listdir(results_dir)
        for f in resfiles:
            if not os.path.isfile(os.path.join(results_dir,f)):
                continue
            if re_tag_dat.match(f):
                dat_files_set.add(f)
            elif re_tag.match(f):
                other_files_set.add(f)
        dat_files = ' '.join(dat_files_set)
        other_files = ' '.join(other_files_set)
        # print "tag: %s got dat files: %s" % (tag, dat_files)
        return (dat_files, other_files)

    def _update_running_job(self, row, force_updt = False):
        # first load job_out info
        start_time = self.jobs_running.get_val(row, "start_time")
        job_out_file = self.jobs_running.get_val(row, "job_out_file")
        tag = self.jobs_running.get_val(row, "tag")

        stdt = datetime.datetime.strptime(start_time, time_format)
        runtime = datetime.datetime.now() - stdt
        
        if force_updt or runtime.seconds < job_update_window * 60:
            # print "job %s running for %d seconds -- updating" % (tag, runtime.seconds)
            job_out = self._get_job_out(job_out_file)
            self.jobs_running.set_val(row, "job_out", job_out)

            all_files = self._get_dat_files(tag)
            self.jobs_running.set_val(row, "dat_files", all_files[0])
            self.jobs_running.set_val(row, "other_files", all_files[1])

    def _move_job_to_done(self, row):
        self._init_jobs_done()
        self.jobs_done.append_row_from(self.jobs_running, row)
        self.jobs_running.remove_row(row)
        self.jobs_done.write(self.cur_done_file)
        self._svn_add_cur_done()

    def _add_cur_running_to_list(self):
        if self.cur_running_file not in self.all_running_files:
            self.all_running_files.add(self.cur_running_file)

    def _svn_add_cur_running(self):
        if self.cur_running_file not in self.all_running_files:
            self.all_running_files.add(self.cur_running_file)
            cmd = ['svn', 'add', '--username', self.username,
                   '--non-interactive', self.cur_running_file]
            # Don't check_output, just dump it to stdout (or nohup.out).
            subprocess.call(cmd)

    def _svn_add_cur_done(self):
        cmd = ['svn', 'add', '--username', self.username,
               '--non-interactive', self.cur_done_file]
        # Don't check_output, just dump it to stdout (or nohup.out).
        subprocess.call(cmd)

    def _commit_changes(self, message='Updates from cluster'):
        # Commit the whole repo (should only be files under 'submit').
        cmd = ['svn', 'ci', '--username', self.username, '-m', message,
               '--non-interactive', self.repo_dir]

        # Don't check_output, just dump it to stdout (or nohup.out).
        subprocess.call(cmd)

#############################################################################

# If the user chooses to run in the background, this file will be created.
# The polling loop will exit if the file is subsequently deleted.  The user
# may delete the file manually, or simply re-running this script will delete
# the file.
nohup_filename = 'nohup-running-monitor-emergent-cluster-runs.txt'

def main():
    # Delete the nohup file, if it exists.
    if os.path.isfile(nohup_filename):
        print 'Removing nohup file: %s' % nohup_filename
        os.remove(nohup_filename)
        print 'The background script should stop at its next poll interval.'
        print 'Hit Ctrl-C at any time if you just want to quit.\n'

    # Read the config file, allow the user to add a new repo, get
    # the repo they'd like to use, and write it all back to disk.
    wc_root = 'emergent-cluster-runs'
    config = ClusterConfig(wc_root)
    username = config.get_username()
    clustername = config.get_clustername()
    repo_name, repo_url = config.choose_repo()

    # Checkout or update the working copy.
    # The path format matches ClusterManager::setPaths() in the C++ code.
    repo_url += '/' + clustername + '/' + username
    repo_dir = os.path.join(wc_root, repo_name)
    print ''
    delay = config.get_poll_interval()
    check_user = config.get_check_user()

    poller = SubversionPoller(username, repo_dir, repo_url, delay, check_user)
    revision = poller.get_initial_wc()

    do_hup = False
    if debug:
        run_nohup = raw_input('\nRun in the background using nohup? [N/y] ')
        if run_nohup and run_nohup in 'yY':
            do_hup = True
    else:
        run_nohup = raw_input('\nRun in the background using nohup? [Y/n] ')
        if not run_nohup or run_nohup in 'yY':
            do_hup = True

    if do_hup:
        cmd = ['nohup', sys.argv[0], username, repo_dir, repo_url, str(delay),
               str(check_user)]
        subprocess.Popen(cmd)
        time.sleep(1) # Give it a chance to start.
        print 'Running in the background.  Re-run script to stop.'
    else:
        poller.poll() # Infinite loop.

def main_background():
    print '\nStarting background run at %s' % datetime.datetime.now()

    username    = sys.argv[1]
    repo_dir    = sys.argv[2]
    repo_url    = sys.argv[3]
    delay       = int(sys.argv[4])
    check_user  = sys.argv[5] == 'True'

    poller = SubversionPoller(username, repo_dir, repo_url, delay, check_user)
    with open(nohup_filename, 'w') as f:
        f.write('delete this file to stop the backgrounded script.')
    poller.poll(nohup_filename) # Infinite loop.

    print '\nStopping background run at %s' % datetime.datetime.now()

#############################################################################

if __name__ == '__main__':
    try:
        if len(sys.argv) == 1:
            main()
        else:
            main_background()
    except KeyboardInterrupt:
        print '\n\nQuitting at user request (Ctrl-C).'
