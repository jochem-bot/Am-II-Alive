import os
import subprocess
import tempfile
import serial
import time
import shutil
import threading
import queue
import pygame
import bluetooth

video_lib = {
    0:	"/home/joche/Downloads/Color Matte_1.mp4",
    1: "/home/joche/Downloads/division/grow/1.mp4",
    2: "/home/joche/Downloads/division/grow/2.mp4",
    3: "/home/joche/Downloads/division/grow/3.mp4",
    4: "/home/joche/Downloads/division/grow/4.mp4",
    5: "/home/joche/Downloads/division/grow/5.mp4",
    6: "/home/joche/Downloads/division/grow/6.mp4",
    7: "/home/joche/Downloads/division/grow/7.mp4",
    8: "/home/joche/Downloads/division/grow/8.mp4",
    9: "/home/joche/Downloads/division/grow/9.mp4",
    10: "/home/joche/Downloads/division/grow/10.mp4",
    11: '/home/joche/Downloads/division/maintain/maintain.mp4'
}

video_reversed = {
    0: "/home/joche/Downloads/division/reverse/10.mp4",
    1: "/home/joche/Downloads/division/reverse/9.mp4",
    2: "/home/joche/Downloads/division/reverse/8.mp4",
    3: "/home/joche/Downloads/division/reverse/7.mp4",
    4: "/home/joche/Downloads/division/reverse/6.mp4",
    5: "/home/joche/Downloads/division/reverse/5.mp4",
    6: "/home/joche/Downloads/division/reverse/4.mp4",
    7: "/home/joche/Downloads/division/reverse/3.mp4",
    8: "/home/joche/Downloads/division/reverse/2.mp4",
    9: "/home/joche/Downloads/division/reverse/1.mp4",
}
videos_2 = {
    0:	"/home/joche/Downloads/Color Matte_1.mp4",
    1: "/home/joche/Downloads/selfsustaining/grow/1.mp4",
    2: "/home/joche/Downloads/selfsustaining/grow/2.mp4",
    3: "/home/joche/Downloads/selfsustaining/grow/3.mp4",
    4: "/home/joche/Downloads/selfsustaining/grow/4.mp4",
    5: "/home/joche/Downloads/selfsustaining/grow/5.mp4",
    6: "/home/joche/Downloads/selfsustaining/grow/6.mp4",
    7: "/home/joche/Downloads/selfsustaining/grow/7.mp4",
    8: "/home/joche/Downloads/selfsustaining/grow/8.mp4",
    9: "/home/joche/Downloads/selfsustaining/grow/9.mp4",
    10: "/home/joche/Downloads/selfsustaining/grow/10.mp4",
    11: '/home/joche/Downloads/selfsustaining/maintain/maintain.mp4'
}

videos_2_reversed = {
    0: "/home/joche/Downloads/selfsustaining/reverse/10.mp4",
    1: "/home/joche/Downloads/selfsustaining/reverse/9.mp4",
    2: "/home/joche/Downloads/selfsustaining/reverse/8.mp4",
    3: "/home/joche/Downloads/selfsustaining/reverse/7.mp4",
    4: "/home/joche/Downloads/selfsustaining/reverse/6.mp4",
    5: "/home/joche/Downloads/selfsustaining/reverse/5.mp4",
    6: "/home/joche/Downloads/selfsustaining/reverse/4.mp4",
    7: "/home/joche/Downloads/selfsustaining/reverse/3.mp4",
    8: "/home/joche/Downloads/selfsustaining/reverse/2.mp4",
    9: "/home/joche/Downloads/selfsustaining/reverse/1.mp4",
}

def read_first_four_sequences(port='/dev/ttyACM0', baudrate=9600, timeout=2):
    try:
        ser = serial.Serial(port, baudrate, timeout=timeout)
        print(f"Connected to {port} at {baudrate} baud.")

        sequences = []
        sequence_count = 0
        reading_sequence = False
        buffer = ""

        while sequence_count < 4:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if not line:
                if reading_sequence:
                    # End of sequence (blank line after data)
                    current_sequence = [int(num) for num in buffer.split(",") if num.strip().isdigit()]
                    sequences.append(current_sequence)
                    sequence_count += 1
                    buffer = ""
                    reading_sequence = False
                continue

            if line.startswith(f"Sequence {sequence_count + 1}"):
                reading_sequence = True
                buffer = ""  # Clear buffer just in case
                continue

            if reading_sequence:
                buffer += line  # Continue adding to the sequence line

        ser.close()
        print("First four sequences received:")
        for idx, seq in enumerate(sequences, start=1):
            print(f"Sequence {idx} (length {len(seq)}): {seq}")

        return sequences

    except serial.SerialException as e:
        print(f"Serial error: {e}")
        return []

def build_video_sequence(sequence, video_lib_local, video_reversed_local):
    video_paths = []
    prev = None

    for idx, current in enumerate(sequence):
        if prev is None:
            path = video_lib_local.get(current)
        elif prev == 10 and current == 10:
            path = video_lib_local.get(current + 1)
            if path:
                video_paths.append(path)
                return video_paths
        elif current >= prev:
            path = video_lib_local.get(current)
        else:
            reversed_key = prev - 1
            path = video_reversed_local.get(reversed_key)

        if path:
            video_paths.append(path)

    # If we're at the last value in the sequence
        if idx == len(sequence) - 1:
            path = video_lib_local.get(current + 1)
            if path:
                video_paths.append(path)
                return video_paths	
        prev = current

    return video_paths


def prepare_video(video_paths, name_prefix, tmpdir):
    list_file_path = os.path.join(tmpdir, f"{name_prefix}_videos.txt")
    temp_video_files = []

    for idx, path in enumerate(video_paths):
        if not os.path.exists(path):
            print(f"Warning: video file {path} not found.")
            continue

        temp_path = os.path.join(tmpdir, f"{name_prefix}_{idx:04d}.mp4")
        if not os.path.exists(temp_path):
            os.symlink(path, temp_path)
        temp_video_files.append(temp_path)

    with open(list_file_path, "w") as f:
        for temp_path in temp_video_files:
            f.write(f"file '{temp_path}'\n")

    output_video = os.path.join(tmpdir, f"{name_prefix}_output.mp4")
    cmd = [
        "ffmpeg",
        "-f", "concat",
        "-safe", "0",
        "-i", list_file_path,
        "-c", "copy",
        output_video
    ]
    print(f"Running ffmpeg to concatenate {name_prefix} video...")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"ffmpeg error for {name_prefix}:")
        print(result.stderr)
        return None
    return output_video

def play_two_videos(video1, video2, timeout): 
    proc1 = subprocess.Popen(["mpv", "--fs", "--screen=0", video1])
    proc2 = subprocess.Popen(["mpv", "--fs", "--screen=1", video2])
    
    start_time = time.time()
    while True:
        if proc1.poll() is not None and proc2.poll() is not None:
            break  
        if time.time() - start_time > timeout:
            print("Time limit reached. Terminating video players.")
            proc1.terminate()
            proc2.terminate()
            break
        time.sleep(1)

def send_sequences(bt_socket, sequences):
    for idx, seq in enumerate(sequences, start=3):
        message = f"Sending Sequence {idx}: {','.join(map(str, seq))}\n"
        bt_socket.send(message.encode('utf-8'))
        time.sleep(0.1)
        



def play(filename, fade_after, override=False):
    global currently_playing, last_play_start

    if override and pygame.mixer.music.get_busy():
        pygame.mixer.music.stop()

    if not pygame.mixer.music.get_busy() or override:
        pygame.mixer.music.load(filename)
        pygame.mixer.music.play()
        last_play_start = time.time()
        currently_playing = (filename, fade_after)

def update_playback():
    global currently_playing, last_play_start
    if currently_playing:
        filename, fade_after = currently_playing
        if time.time() - last_play_start >= fade_after:
            if pygame.mixer.music.get_busy():
                pygame.mixer.music.fadeout(2000)
            currently_playing = None



            



target_address = "2C:CF:67:75:75:50"
port = 1
bt_socket = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
bt_socket.connect((target_address, port))
print("Connected to receiver over Bluetooth.")



def main_loop():
    print("Starting video loop. Press Ctrl+C to stop.")
    os.system('wmctrl -r "Thonny  -  /home/joche/latest.py" -b add,hidden')
    os.system("pkill lxpanel")
    
    played = False

    try:
        while True:
            
        
            ser2 = serial.Serial('/dev/ttyACM0', 9600)  # adjust port as neede
            sequences = read_first_four_sequences()
            time.sleep(1)
            ser2.write(b'R')
            

            if not sequences or len(sequences) < 4:
                print("Fewer than 4 sequences received. Waiting for next...")
                time.sleep(1)
                continue


            try:
                send_sequences(bt_socket, sequences[2:4])
                start_time = time.time()
                print("Sent sequences 3 and 4 over Bluetooth.")
                
                played = False
            except bluetooth.BluetoothError as e:
                print(f"Error sending sequences over Bluetooth: {e}")

            video_paths_1 = build_video_sequence(sequences[0], video_lib, video_reversed)
            video_paths_2  = build_video_sequence(sequences[1], videos_2, videos_2_reversed)

            with tempfile.TemporaryDirectory() as tmpdir:
                video1 = prepare_video(video_paths_1, "seq1", tmpdir)
                video2 = prepare_video(video_paths_2, "seq2", tmpdir)
                print("we zijn klaar", time.time() - start_time)
                if not video1 or not video2:
                    print("Error creating output videos.")
                    continue

                while not played:
                    current_time = time.time()
                    time.sleep(0.1)
                    if current_time - start_time >= 60:
                        print("75 seconds passed. Playing videos now.")
                        print(360-current_time+start_time)
                        play_two_videos(video1, video2, 360-current_time+start_time)
                        played = True
                       

            
  

    except KeyboardInterrupt:
        print("\nStopped by user.")




if _name_ == "_main_":
    main_loop()

client_sock.close()
server_sock.close()