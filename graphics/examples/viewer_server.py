# viewer_client.py - Simple client to receive plot images from SocketPlotDisplay.jl server
import socket
import struct
import os
import sys

def receive_plots(host="127.0.0.1", port=9000, output_dir="received_plots"):
    """
    Connect to a SocketPlotDisplay.jl server and receive plot images.

    Args:
        host: Server hostname/IP
        port: Server port
        output_dir: Directory to save received images
    """
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)

    print(f"Connecting to {host}:{port}...")

    try:
        # Create socket and connect to server
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))
        print(f"Connected to {host}:{port}")

        count = 0
        while True:
            # Read exactly 8 bytes for the length header
            hdr = b""
            while len(hdr) < 8:
                chunk = sock.recv(8 - len(hdr))
                if not chunk:
                    print("Connection closed by server")
                    return
                hdr += chunk

            # Unpack big-endian uint64 length
            length = struct.unpack(">Q", hdr)[0]
            print(f"Receiving image {count}, size: {length} bytes")

            # Sanity check
            if length <= 0 or length > 50_000_000:  # 50MB limit
                print(f"Invalid length: {length}, stopping")
                break

            # Read the image data
            data = b""
            while len(data) < length:
                remaining = length - len(data)
                chunk = sock.recv(min(remaining, 8192))  # Read in chunks
                if not chunk:
                    print("Connection closed while receiving data")
                    return
                data += chunk

            # Save the image
            fname = os.path.join(output_dir, f"plot_{count:04d}.png")
            with open(fname, "wb") as f:
                f.write(data)
            print(f"✅ Saved {fname}")

            count += 1

    except KeyboardInterrupt:
        print("\nInterrupted by user")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        try:
            sock.close()
        except:
            pass
        print("Connection closed")

if __name__ == "__main__":
    # Allow command line arguments
    host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 9000
    output_dir = sys.argv[3] if len(sys.argv) > 3 else "received_plots"

    print("SocketPlotDisplay.jl Image Receiver")
    print("===================================")
    receive_plots(host, port, output_dir)
