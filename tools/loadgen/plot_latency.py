#!/usr/bin/env python3

"""
Simple latency plotting script for load test results.
Usage: python3 plot_latency.py <hey_output_file> [output_image]
"""

import sys
import re
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime

def parse_hey_output(filename):
    """Parse hey output and extract latency metrics."""
    with open(filename, 'r') as f:
        content = f.read()
    
    # Extract latency percentiles
    latency_pattern = r'(\d+(?:\.\d+)?)([a-zA-Z]+)'
    
    # Find latency section
    latency_section = re.search(r'Latency distribution:(.*?)Requests/sec:', content, re.DOTALL)
    if not latency_section:
        print("Error: Could not find latency distribution in output")
        return None
    
    latency_text = latency_section.group(1)
    
    # Parse percentiles
    percentiles = {}
    for line in latency_text.split('\n'):
        if '%' in line:
            parts = line.strip().split()
            if len(parts) >= 2:
                percentile = parts[0].rstrip('%')
                try:
                    value = float(parts[1])
                    percentiles[float(percentile)] = value
                except ValueError:
                    continue
    
    # Extract summary stats
    total_requests = re.search(r'Total:\s+(\d+)\s+requests', content)
    duration = re.search(r'in\s+([\d.]+)s', content)
    req_per_sec = re.search(r'Requests/sec:\s+([\d.]+)', content)
    
    stats = {
        'total_requests': int(total_requests.group(1)) if total_requests else 0,
        'duration': float(duration.group(1)) if duration else 0,
        'requests_per_sec': float(req_per_sec.group(1)) if req_per_sec else 0,
        'percentiles': percentiles
    }
    
    return stats

def create_latency_plot(stats, output_file='latency_plot.png'):
    """Create a latency percentile plot."""
    if not stats or not stats['percentiles']:
        print("Error: No latency data to plot")
        return
    
    percentiles = stats['percentiles']
    
    # Create figure
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
    
    # Plot 1: Latency percentiles
    p_values = list(percentiles.keys())
    latencies = list(percentiles.values())
    
    ax1.plot(p_values, latencies, 'bo-', linewidth=2, markersize=6)
    ax1.set_xlabel('Percentile')
    ax1.set_ylabel('Latency (ms)')
    ax1.set_title('Latency Distribution')
    ax1.grid(True, alpha=0.3)
    ax1.set_yscale('log')
    
    # Add labels for key percentiles
    key_percentiles = [50, 90, 95, 99]
    for p in key_percentiles:
        if p in percentiles:
            ax1.axvline(x=p, color='red', linestyle='--', alpha=0.5)
            ax1.text(p, percentiles[p], f'P{p}: {percentiles[p]:.1f}ms', 
                    rotation=90, verticalalignment='bottom')
    
    # Plot 2: Summary stats
    summary_text = f"""
    Load Test Summary
    Total Requests: {stats['total_requests']:,}
    Duration: {stats['duration']:.1f}s
    Requests/sec: {stats['requests_per_sec']:.1f}
    
    Key Latencies:
    P50: {percentiles.get(50, 0):.1f}ms
    P90: {percentiles.get(90, 0):.1f}ms
    P95: {percentiles.get(95, 0):.1f}ms
    P99: {percentiles.get(99, 0):.1f}ms
    """
    
    ax2.text(0.1, 0.9, summary_text, transform=ax2.transAxes, 
            fontsize=12, verticalalignment='top',
            bbox=dict(boxstyle='round', facecolor='lightgray', alpha=0.8))
    ax2.axis('off')
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Latency plot saved to: {output_file}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 plot_latency.py <hey_output_file> [output_image]")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else 'latency_plot.png'
    
    try:
        stats = parse_hey_output(input_file)
        if stats:
            create_latency_plot(stats, output_file)
        else:
            print("Failed to parse hey output")
            sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()

