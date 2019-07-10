#!/usr/bin/env python

# Simon Duquennoy (simonduq@sics.se)

import sys
import os
import subprocess
import string

# extract node_id from file name (e.g. "pi16" -> 16)
extract_pi_number = lambda x: int(filter(str.isdigit, x) or 0)

def get_next_line(log_files):
  curr_earliest = None
  # look for earliest timestamp
  for f in log_files:
    if f['next_timestamp'] != None:
      if curr_earliest == None or curr_earliest['next_timestamp'] > f['next_timestamp']:
        curr_earliest = f
        
  if curr_earliest != None:
    # store current message
    timestamp = curr_earliest['next_timestamp']
    node_id = curr_earliest['node_id']
    message = curr_earliest['next_message']
    # get next
    next_timestamp, next_message = parse_next_line(curr_earliest['file'])
    curr_earliest['next_timestamp'] = next_timestamp
    curr_earliest['next_message'] = next_message
    # return current
    return timestamp, node_id, message
  else:
    return None, None, None

def parse_next_line(file):
  while True:
    # extract timestamp and message from line
    try:
      line = file.next()
      timestamp, message = line.split(None, 1)
      timestamp = int(timestamp)
      # if the log starts with zeros (happens with the first line), filter out non-printable characters
      if ord(message[0]) == 0:
        message = filter(lambda x: x in string.printable, message)
      return timestamp, message
    except StopIteration:
	    return None, None
    except:
      continue

if __name__=="__main__":
  
  if len(sys.argv)<2:
    print "Job directory parameter not found!"
    sys.exit(1)
    
  # The only parameter contains the job directory
  job_dir = sys.argv[1]
         
  # a dict containing all log lines keyed with timestamps
  all_lines = dict()
  
  # the file containing the aggregated logs
  dest_file_path = os.path.join(job_dir, "logs", "log.txt")
  dest_file = open(dest_file_path, 'w')
  
  # the directory containing the logs from all nodes
  logs_dir = os.path.join(job_dir, "logs")
  # first open every individual log file
  log_files = []
  print "Aggregating logs...",
  sys.stdout.flush()
  for f in os.listdir(logs_dir):
    if os.path.isdir(os.path.join(logs_dir, f)):
      # extract node_id from file name (e.g. "pi16" -> 16)
      node_id = extract_pi_number(f)

      log_file_path = os.path.join(logs_dir, f, "log.txt")
      curr_log_file = open(log_file_path, 'r')
	  
      timestamp, message = parse_next_line(curr_log_file)
      log_files.append({'node_id': node_id, 'file': curr_log_file, 'next_timestamp': timestamp, 'next_message': message})
  
  line_cnt = 0
  while True:
    timestamp, node_id, message = get_next_line(log_files)
    if timestamp == None:
      break
    # generate a single aggregated log file  
    dest_file.write("%016u\tID:%u\t%s"%(timestamp, node_id, message))
    line_cnt += 1
    if line_cnt % 100000 == 0:
      print "%u"%(line_cnt),
      sys.stdout.flush()
	  
  print "done (%u lines)" %(line_cnt)
