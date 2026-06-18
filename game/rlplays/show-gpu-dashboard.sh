#!/bin/bash
# Run all scripts from game/ directory.
# filepath: nvidia_dashboard.sh

# Function to display nvidia-smi output with timestamp
show_nvidia_info() {
    clear
    echo "=========================================="
    echo "NVIDIA GPU Dashboard - $(date '+%Y-%m-%d %H:%M:%S')"
    echo "=========================================="
    echo ""
    
    # Check if nvidia-smi is available
    if command -v nvidia-smi &> /dev/null; then
        nvidia-smi
    else
        echo "Error: nvidia-smi not found. Please ensure NVIDIA drivers are installed."
        exit 1
    fi
    
    echo ""
    echo "----------------------------------------"
    echo "Refreshing every 4 seconds... (Press Ctrl+C to exit)"
    echo "----------------------------------------"
}

# Trap Ctrl+C to exit gracefully
trap 'echo -e "\n\nDashboard stopped."; exit 0' SIGINT

# Main loop
while true; do
    show_nvidia_info
    sleep 4
done
