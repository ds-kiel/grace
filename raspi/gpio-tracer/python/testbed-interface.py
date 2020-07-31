# This implements the logic that will have to be implemented in the real testbed script later
# This file collects all logs from all overseer nodes and combines them into one csv file.
# It will also gather the factors from the sync nodes and scale the collected data from the overseers
# accordenly.
import requests, csv

overseer_node_ids = [20, 5, 7]
sync_node_ids = [15]
log_contents = {}
combined_log_content = [];
sync_timestamps = {};

for overseer_id in overseer_node_ids:
    log_content = requests.get("http://raspi{:0>2}:5000/log".format(overseer_id)).content.decode()
    log_contents[overseer_id] = log_content.splitlines()

for sync_id in sync_node_ids:
    timestamp_pair = requests.get("http://raspi{:0>2}:5000/timestamps".format(sync_id)).content.decode()
    print(str(timestamp_pair))
    sync_timestamps[]

def decomment(csvfile):
    for row in csvfile:
        raw = row.split('#')[0].strip()
        if raw: yield raw

for overseer_id,log_content in log_contents.items():
    csv_log_content = csv.reader(decomment(log_content), delimiter=',', quotechar='|')
    for row in csv_log_content:
        pin_id         = row[0]
        digital_signal = row[1]
        ns_timestamp   = row[2]
        combined_log_content.append([overseer_id, pin_id, digital_signal, ns_timestamp])

combined_log_content_sorted = sorted(combined_log_content, key=lambda row: row[-1])
