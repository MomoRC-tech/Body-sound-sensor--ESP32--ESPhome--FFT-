"""
Example Python script to parse and analyze FFT spectrum JSON from Home Assistant.

This script demonstrates how to:
1. Connect to Home Assistant via REST API or WebSocket
2. Parse the JSON spectrum data
3. Visualize band energies
4. Log data for ML training
"""

import json
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime

# Example JSON spectrum data (as received from the text sensor)
example_json = '''
{
  "fs": 1000.0,
  "n": 512,
  "bin_hz": 1.953,
  "rms": 0.012345,
  "peak_hz": 49.2,
  "bands": [12.5, 8.3, 15.7, 22.1, 18.9, 11.2, 9.4, 7.8, 6.5, 5.2, 4.1, 3.3, 2.8, 2.1, 1.5, 1.2]
}
'''


def parse_spectrum(json_str):
    """Parse JSON spectrum string into structured data."""
    try:
        data = json.loads(json_str)
        return data
    except json.JSONDecodeError as e:
        print(f"Error parsing JSON: {e}")
        return None


def compute_band_frequencies(fs, n_bands):
    """Compute center frequencies for each band."""
    f_max = fs / 2.0  # Nyquist frequency
    band_width = f_max / n_bands
    frequencies = [(i + 0.5) * band_width for i in range(n_bands)]
    return frequencies


def visualize_spectrum(spectrum_data):
    """Create a bar chart visualization of band energies."""
    bands = spectrum_data['bands']
    fs = spectrum_data['fs']
    n_bands = len(bands)
    
    # Compute center frequencies for each band
    frequencies = compute_band_frequencies(fs, n_bands)
    
    # Create figure
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    
    # Plot 1: Band energies
    ax1.bar(frequencies, bands, width=frequencies[1]-frequencies[0], 
            edgecolor='black', alpha=0.7)
    ax1.set_xlabel('Frequency (Hz)')
    ax1.set_ylabel('Energy (arbitrary units)')
    ax1.set_title(f'Spectrum Bands (RMS: {spectrum_data["rms"]:.4f}g, Peak: {spectrum_data["peak_hz"]:.1f}Hz)')
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: Band energies in dB scale (log)
    bands_db = 10 * np.log10(np.array(bands) + 1e-10)  # Avoid log(0)
    ax2.bar(frequencies, bands_db, width=frequencies[1]-frequencies[0], 
            edgecolor='black', alpha=0.7, color='orange')
    ax2.set_xlabel('Frequency (Hz)')
    ax2.set_ylabel('Energy (dB)')
    ax2.set_title('Spectrum Bands (dB scale)')
    ax2.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.show()


def classify_device(spectrum_data):
    """
    Simple rule-based classification example.
    
    This demonstrates how to use band energies to identify devices.
    For real applications, use machine learning models trained on labeled data.
    """
    bands = np.array(spectrum_data['bands'])
    rms = spectrum_data['rms']
    peak_hz = spectrum_data['peak_hz']
    
    # Example rules (adjust thresholds based on your data)
    
    # Rule 1: Pump detection (50Hz region, high RMS)
    if 45 <= peak_hz <= 55 and rms > 0.02:
        if bands[2] > 15 and bands[3] > 15:  # Bands around 50Hz
            return "Well Pump Active"
    
    # Rule 2: Fan detection (broader spectrum, medium RMS)
    if rms > 0.01 and rms < 0.03:
        if np.mean(bands[4:8]) > 8:  # Mid-frequency bands
            return "Ventilation Fan Active"
    
    # Rule 3: Idle state (low RMS)
    if rms < 0.005:
        return "Background / Idle"
    
    return "Unknown Device"


def log_spectrum_to_file(spectrum_data, label, filename='spectrum_log.csv'):
    """
    Log spectrum data with label to CSV file for ML training.
    
    Args:
        spectrum_data: Parsed spectrum dictionary
        label: String label (e.g., "pump_on", "fan_on", "idle")
        filename: Output CSV filename
    """
    import csv
    from pathlib import Path
    
    file_exists = Path(filename).exists()
    
    with open(filename, 'a', newline='') as f:
        writer = csv.writer(f)
        
        # Write header if new file
        if not file_exists:
            header = ['timestamp', 'label', 'rms', 'peak_hz']
            header.extend([f'band_{i}' for i in range(len(spectrum_data['bands']))])
            writer.writerow(header)
        
        # Write data row
        row = [
            datetime.now().isoformat(),
            label,
            spectrum_data['rms'],
            spectrum_data['peak_hz']
        ]
        row.extend(spectrum_data['bands'])
        writer.writerow(row)
    
    print(f"Logged spectrum with label '{label}' to {filename}")


def extract_features(spectrum_data):
    """
    Extract additional features from spectrum for ML models.
    
    Returns a feature vector suitable for classification.
    """
    bands = np.array(spectrum_data['bands'])
    
    features = {
        'rms': spectrum_data['rms'],
        'peak_hz': spectrum_data['peak_hz'],
        'band_mean': np.mean(bands),
        'band_std': np.std(bands),
        'band_max': np.max(bands),
        'band_min': np.min(bands),
        'spectral_centroid': np.sum(bands * np.arange(len(bands))) / np.sum(bands),
        'spectral_rolloff': np.percentile(bands, 85),
        'low_freq_energy': np.sum(bands[:4]),   # 0-125 Hz
        'mid_freq_energy': np.sum(bands[4:12]),  # 125-375 Hz
        'high_freq_energy': np.sum(bands[12:]),  # 375-500 Hz
    }
    
    # Add individual bands
    for i, band_val in enumerate(bands):
        features[f'band_{i}'] = band_val
    
    return features


def connect_to_homeassistant_websocket():
    """
    Example: Connect to Home Assistant WebSocket API to receive real-time updates.
    
    Requires: pip install websocket-client
    """
    import websocket
    import json
    
    # Configuration
    HA_URL = "ws://homeassistant.local:8123/api/websocket"
    HA_TOKEN = "YOUR_LONG_LIVED_ACCESS_TOKEN"
    ENTITY_ID = "text_sensor.body_sound_spectrum_json"
    
    def on_message(ws, message):
        msg = json.loads(message)
        
        # Handle authentication
        if msg.get('type') == 'auth_required':
            ws.send(json.dumps({
                'type': 'auth',
                'access_token': HA_TOKEN
            }))
            return
        
        # Handle state changes
        if msg.get('type') == 'event':
            event = msg.get('event', {})
            if event.get('event_type') == 'state_changed':
                entity_id = event['data']['entity_id']
                if entity_id == ENTITY_ID:
                    new_state = event['data']['new_state']['state']
                    spectrum_data = parse_spectrum(new_state)
                    if spectrum_data:
                        print(f"Received spectrum: RMS={spectrum_data['rms']:.4f}g, Peak={spectrum_data['peak_hz']:.1f}Hz")
                        device = classify_device(spectrum_data)
                        print(f"  Classification: {device}")
    
    def on_error(ws, error):
        print(f"Error: {error}")
    
    def on_close(ws, close_status_code, close_msg):
        print("WebSocket closed")
    
    def on_open(ws):
        print("WebSocket connected")
        # Subscribe to state changes
        ws.send(json.dumps({
            'id': 1,
            'type': 'subscribe_events',
            'event_type': 'state_changed'
        }))
    
    ws = websocket.WebSocketApp(HA_URL,
                                on_open=on_open,
                                on_message=on_message,
                                on_error=on_error,
                                on_close=on_close)
    
    print(f"Connecting to {HA_URL}...")
    ws.run_forever()


def main():
    """Main example demonstrating spectrum analysis."""
    
    print("=== ESP32 Vibration FFT Spectrum Analyzer ===\n")
    
    # Parse example JSON
    spectrum_data = parse_spectrum(example_json)
    
    if spectrum_data:
        print("Parsed spectrum data:")
        print(f"  Sample rate: {spectrum_data['fs']} Hz")
        print(f"  FFT size: {spectrum_data['n']}")
        print(f"  Frequency resolution: {spectrum_data['bin_hz']:.3f} Hz")
        print(f"  RMS vibration: {spectrum_data['rms']:.6f} g")
        print(f"  Peak frequency: {spectrum_data['peak_hz']:.2f} Hz")
        print(f"  Number of bands: {len(spectrum_data['bands'])}")
        print()
        
        # Classify device
        device = classify_device(spectrum_data)
        print(f"Device classification: {device}")
        print()
        
        # Extract features
        features = extract_features(spectrum_data)
        print("Extracted features:")
        for key, value in features.items():
            if not key.startswith('band_'):
                print(f"  {key}: {value:.4f}")
        print()
        
        # Log to file (commented out by default)
        # log_spectrum_to_file(spectrum_data, label="example", filename="spectrum_log.csv")
        
        # Visualize
        print("Generating visualization...")
        visualize_spectrum(spectrum_data)
    
    # Uncomment to connect to Home Assistant WebSocket
    # connect_to_homeassistant_websocket()


if __name__ == "__main__":
    main()
