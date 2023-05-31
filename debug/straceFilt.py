with open("strace.log", "r") as input_file, open("filtered.log", "w") as output_file:
    for line in input_file:
        if any(call in line for call in ["open", "close", "read", "write", "rename"]):
            output_file.write(line)