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
    0: "/home/joche/Downloads/WhatsApp Video 2025-06-11 at 10.36.15.mp4",
    1: "/home/joche/Downloads/evolving/grow/1.mp4",
    2: "/home/joche/Downloads/evolving/grow/2.mp4",
    3: "/home/joche/Downloads/evolving/grow/3.mp4",
    4: "/home/joche/Downloads/evolving/grow/4.mp4",
    5: "/home/joche/Downloads/evolving/grow/5.mp4",
    6: "/home/joche/Downloads/evolving/grow/6.mp4",
    7: "/home/joche/Downloads/evolving/grow/7.mp4",
    8: "/home/joche/Downloads/evolving/grow/8.mp4",
    9: "/home/joche/Downloads/evolving/grow/9.mp4",
    10: "/home/joche/Downloads/evolving/grow/10.mp4",
    11: "/home/joche/Downloads/evolving/maintain/maintain.mp4"
}

video_reversed = {
    0: "/home/joche/Downloads/evolving/reverse/10.mp4",
    1: "/home/joche/Downloads/evolving/reverse/9.mp4",
    2: "/home/joche/Downloads/evolving/reverse/8.mp4",
    3: "/home/joche/Downloads/evolving/reverse/7.mp4",
    4: "/home/joche/Downloads/evolving/reverse/6.mp4",
    5: "/home/joche/Downloads/evolving/reverse/5.mp4",
    6: "/home/joche/Downloads/evolving/reverse/4.mp4",
    7: "/home/joche/Downloads/evolving/reverse/3.mp4",
    8: "/home/joche/Downloads/evolving/reverse/2.mp4",
    9: "/home/joche/Downloads/evolving/reverse/1.mp4",
}
videos_2 = {
    0: "/home/joche/Downloads/WhatsApp Video 2025-06-11 at 10.36.15.mp4",
    1: "/home/joche/Downloads/sensingandresponse/grow/1.mp4",
    2: "/home/joche/Downloads/sensingandresponse/grow/2.mp4",
    3: "/home/joche/Downloads/sensingandresponse/grow/3.mp4",
    4: "/home/joche/Downloads/sensingandresponse/grow/4.mp4",
    5: "/home/joche/Downloads/sensingandresponse/grow/5.mp4",
    6: "/home/joche/Downloads/sensingandresponse/grow/6.mp4",
    7: "/home/joche/Downloads/sensingandresponse/grow/7.mp4",
    8: "/home/joche/Downloads/sensingandresponse/grow/8.mp4",
    9: "/home/joche/Downloads/sensingandresponse/grow/9.mp4",
    10: "/home/joche/Downloads/sensingandresponse/grow/10.mp4",
    11: '/home/joche/Downloads/sensingandresponse/maintain/maintain.mp4'
}

videos_2_reversed = {
    0: "/home/joche/Downloads/sensingandresponse/reverse/10.mp4",
    1: "/home/joche/Downloads/sensingandresponse/reverse/9.mp4",
    2: "/home/joche/Downloads/sensingandresponse/reverse/8.mp4",
    3: "/home/joche/Downloads/sensingandresponse/reverse/7.mp4",
    4: "/home/joche/Downloads/sensingandresponse/reverse/6.mp4",
    5: "/home/joche/Downloads/sensingandresponse/reverse/5.mp4",
    6: "/home/joche/Downloads/sensingandresponse/reverse/4.mp4",
    7: "/home/joche/Downloads/sensingandresponse/reverse/3.mp4",
    8: "/home/joche/Downloads/sensingandresponse/reverse/2.mp4",
    9: "/home/joche/Downloads/sensingandresponse/reverse/1.mp4",
}


def read_sequences_bt(client_sock, num_sequences=2, buffer_size=1024):
    sequences = []
    data_accumulator = ""

    try:
        while len(sequences) < num_sequences:
            data = client_sock.recv(buffer_size).decode('utf-8', errors='ignore')
            if not data:
                print("Connection closed by sender.")
                break

            data_accumulator += data

            while "\n" in data_accumulator:
                line, data_accumulator = data_accumulator.split("\n", 1)
                line = line.strip()
                if not line:
                    continue

                try:
                    sequence = [int(num) for num in line.split(",") if num.strip().isdigit()]
                    if sequence:
                        sequences.append(sequence)
                        print(f"Received Sequence {len(sequences)}: {sequence}")
                except ValueError:
                    print(f"Invalid line (not a sequence): {line}")

    except bluetooth.BluetoothError as e:
        print(f"Bluetooth error: {e}")

    return sequences



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

def play_two_videos(video1, video2, timeout=300): 
    proc1 = subprocess.Popen(["mpv", "--fs", "--screen=0", video1])
    proc2 = subprocess.Popen(["mpv", "--fs", "--screen=1", video2])
    
    start_time = time.time()
    while True:
        if proc1.poll() is not None and proc2.poll() is not None:
            break  # both finished on their own
        if time.time() - start_time > timeout:
            print("Time limit reached. Terminating video players.")
            proc1.terminate()
            proc2.terminate()
            break
        time.sleep(1)

def send_sequences(ser, sequences):
    for idx, seq in enumerate(sequences, start=3):
        message = f"Sending Sequence {idx}: {','.join(map(str, seq))}\n"
        ser.write(message.encode('utf-8'))
        time.sleep(0.1)  

pygame.mixer.init()
last_play_start = 0
currently_playing = None

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


class SoundPlayer:
    def _init_(self):
        self.currently_playing = None  # (filename, fade_after, start_time, fade_in_ms)
        self.fading_out = False
        self.fade_duration_ms = 2000  # fade-out time in ms

    def play(self, filename, fade_after, override=False, fade_in_ms=0):
        """
        Play a sound file with optional fade-in and fade-out.

        :param filename: path to the audio file
        :param fade_after: time in seconds before starting fade-out
        :param override: if True, stops and replaces current sound
        :param fade_in_ms: duration of fade-in in milliseconds
        """
        if override or not pygame.mixer.music.get_busy():
            pygame.mixer.music.stop()
            pygame.mixer.music.load(filename)
            pygame.mixer.music.play(fade_ms=fade_in_ms)
            self.currently_playing = (filename, fade_after, time.time(), fade_in_ms)
            self.fading_out = False

    def update(self):
        """
        Check if it's time to fade out, and update the state accordingly.
        """
        if self.currently_playing:
            filename, fade_after, start_time, fade_in_ms = self.currently_playing
            elapsed = time.time() - start_time
            if elapsed >= fade_after and not self.fading_out:
                pygame.mixer.music.fadeout(self.fade_duration_ms)
                self.fading_out = True
            elif self.fading_out and not pygame.mixer.music.get_busy():
                self.currently_playing = None
                self.fading_out = False

sound_player = SoundPlayer()

def send_oke(bt_socket):
    """
    Sends the response 'oke' over the given Bluetooth socket.

    Args:
        bt_socket: The Bluetooth socket to send the response through.
    """
    try:
        bt_socket.sendall(b"oke")
        print("Sent 'oke' over Bluetooth.")
    except Exception as e:
        print(f"Failed to send 'oke': {e}")



port = 1
server_sock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
server_sock.bind(("", port))
server_sock.listen(1)

print("Waiting for Bluetooth connection on RFCOMM channel", port)
client_sock, client_info = server_sock.accept()
print(f"Accepted Bluetooth connection from {client_info}")


def main_loop():
    print("Starting video loop. Press Ctrl+C to stop.")
    os.system('wmctrl -r "Thonny  -  /home/joche/latest.py" -b add,hidden')
    os.system("pkill lxpanel")
            
    played = False
    

    try:
        while True:
            
            sequences = read_sequences_bt(client_sock, num_sequences=2)
            print('received')
            start_time = time.time()
            played= False
            if not sequences or len(sequences) < 2:
                print("Fewer than 4 sequences received. Waiting for next...")
                time.sleep(1)
                continue



            video_paths_1 = build_video_sequence(sequences[0], video_lib, video_reversed)
            video_paths_2  = build_video_sequence(sequences[1], videos_2, videos_2_reversed)

            with tempfile.TemporaryDirectory() as tmpdir:
                video1 = prepare_video(video_paths_1, "seq1", tmpdir)
                video2 = prepare_video(video_paths_2, "seq2", tmpdir)



                while not played:
                    current_time = time.time()
                    if current_time - start_time >= 60:
                        print("75 seconds passed. Playing videos now.")

                        play_two_videos(video1, video2, 360-current_time+start_time)
                        played = True
                       

            sound_player.play("/home/joche/Downloads/S.C Sounds/4_Restart.mp3", 10)
    except KeyboardInterrupt:
        print("\nStopped by user.")




if _name_ == "_main_":
    main_loop()



client_sock.close()
server_sock.close()