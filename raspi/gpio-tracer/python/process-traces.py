import csv,sys,os

hosts = [
    "raspi09",
    "raspi20",
    "raspi03",
    "raspi07",
    "raspi16",
    "raspi15",
    "raspi12",
    "raspi06",
    "raspi10",
    ]

channels = [i for i  in range(1, 9)]


# TODO merge traces with the same timestamp
def read_traces(logs_dir):
    traces = []
    for host in hosts:
        file_cnt = 0
        traces_folder = os.path.join(logs_dir, host + "/", "traces/")
        while file_cnt >= 0:
            current_file = os.path.join(traces_folder, "traces-" + str(file_cnt))
            try:
                f = open(current_file, "rb")
                try:
                    bytes = f.read(-1)
                    for k in range(0, len(bytes), 10):
                        channel = bytes[k]
                        state = bytes[k+1]
                        timestamp = int.from_bytes(bytes[k+2:k+10], "little")
                        traces.append([timestamp, host, channel, state])
                finally:
                    f.close()
                    file_cnt += 1
            except:
                print("cant open file {}".format(current_file))
                file_cnt = -1
    sorted_traces = sorted(traces, key=lambda x: x[0])
    min_timestamp = sorted_traces[0][0]
    shifted_traces = map(lambda trace: [trace[0]-min_timestamp, trace[1], trace[2], trace[3]], sorted_traces)
    return list(shifted_traces)


# # merges list of timestamps
# def merge_by_timestamp():

def write_csv(traces, output_file_path):
    with open(output_file_path, 'w', newline='') as csvfile:
        header = ["timestamp", "host", "channel", "state"]
        out_writer = csv.writer(csvfile, delimiter=',',
                                quotechar='|', quoting=csv.QUOTE_MINIMAL,
                                )

        out_writer.writerow(header)
        out_writer.writerows(traces)


def write_vcd(traces, output_file_path): # todo use argument
    chars = ["#", "$", "%", "&", "'", "(", ")", "^"]
    syms = []
    for k in chars:
        for i in chars:
            syms.append(k+i)
    with open(output_file_path, "w") as f:

        # Write header
        header_begin = ["$timescale 1ns $end\n",
                        "$scope module logic $end\n"]
        f.writelines(header_begin)
        for k in range(len(hosts)):
            for chan in channels:
                host_traces = list(filter(lambda trace: trace[1]==hosts[k] and trace[2]==chan, traces))
                if len(host_traces) > 0:
                    f.write("$var wire 1 {}{} {} $end\n".format(syms[k], syms[chan-1],  "{}-ch{}".format(hosts[k], str(chan))))

        header_end = ["$upscope $end\n",
                      "$enddefinitions $end\n",
                      "$dumpvars\n"]
        f.writelines(header_end)

        # Write traces
        for trace in traces:
            channel = trace[2]
            host = trace[1]
            state = trace[3]
            f.write("#{}\n".format(trace[0]))
            f.write("{}{}{}\n".format(state, syms[hosts.index(host)], syms[channel-1]))

if __name__=="__main__":
    if len(sys.argv) < 2:
        print("Too few arguments passed. Pass Raspberry id, trace directory and output csv file as arguments to the script")
        sys.exit(1)

    logs_dir = sys.argv[1]

    csv_output_location = os.path.join(logs_dir, "combined-traces.csv")
    vcd_output_location = os.path.join(logs_dir, "combined-traces.vcd")

    # accumulate traces from files
    traces = read_traces(logs_dir)
    # write traces into csv
    write_csv(traces, csv_output_location)
    # write traces into vcd
    write_vcd(traces, vcd_output_location)

    print("done processing traces")
