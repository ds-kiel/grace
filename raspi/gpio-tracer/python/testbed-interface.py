# This implements the logic that will have to be implemented in the real testbed script later
# This file collects all logs from all overseer nodes and combines them into one csv file.
# It will also gather the factors from the sync nodes and scale the collected data from the overseers
# accordenly.
import requests, csv,json

overseer_node_ids = [20, 5, 7]
receiver_pin = 1
sync_node_ids = [15]
log_contents = {}
combined_log_content = [];
all_sync_timestamps = {};

def decomment(csvfile):
    for row in csvfile:
        raw = row.split('#')[0].strip()
        if raw: yield raw

for overseer_id in overseer_node_ids:
    log_content = requests.get("http://raspi{:0>2}:5000/log".format(overseer_id)).content.decode()
    log_contents[overseer_id] = log_content.splitlines()

for sync_id in sync_node_ids:
    response = requests.get("http://raspi{:0>2}:5000/timestamps".format(sync_id))
    sync_timestamps = response.json()
    # json only knows string indices
    sync_timestamps_fixed_key = {int(key): value for key, value in sync_timestamps.items()}
    all_sync_timestamps = {**all_sync_timestamps, **sync_timestamps_fixed_key}
    print(all_sync_timestamps)

for overseer_id,log_content in log_contents.items():
    # here also a dict reader could be used. In this case uncomment the first line GPIO, Edge, Time
    csv_log_content = csv.reader(decomment(log_content), delimiter=',', quotechar='|')
    timestamps = list(map(lambda row: int(row[2]), filter(lambda row: int(row[0]) == receiver_pin, csv_log_content)))
    print(timestamps)

    # TODO generalise to arbritary amount of sync timestamps
    scale_factor = (all_sync_timestamps[overseer_id][1]-all_sync_timestamps[overseer_id][0])/timestamps[1]

    csv_log_content = csv.reader(decomment(log_content), delimiter=',', quotechar='|')
    for row in csv_log_content:
        pin_id         = int(row[0])
        digital_signal = int(row[1])
        raw_timestamp   = int(row[2])
        scaled_timestamp = raw_timestamp * scale_factor
        combined_timestamp = scaled_timestamp + all_sync_timestamps[overseer_id][0]
        combined_log_content.append([overseer_id, pin_id, digital_signal, int(combined_timestamp)])

combined_log_content_sorted = sorted(combined_log_content, key=lambda row: row[-1])
print(str(combined_log_content_sorted))

with open('testrun-gpio-traces.csv', 'w', newline='') as csvfile:
    spamwriter = csv.writer(csvfile, delimiter=' ',
                            quotechar='|', quoting=csv.QUOTE_MINIMAL)
    spamwriter.writerows(combined_log_content_sorted)
