package com.github.jamulussoftware.jamulus;

import android.content.Context;
import android.content.pm.PackageManager;
import android.media.midi.MidiDevice;
import android.media.midi.MidiDeviceInfo;
import android.media.midi.MidiInputPort;
import android.media.midi.MidiManager;
import android.media.midi.MidiReceiver;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Android MIDI Manager for Jamulus
 * Provides MIDI input functionality for Android devices
 */
public class JamulusMidiManager {
    private static final String TAG = "JamulusMidiManager";
    
    private Context mContext;
    private MidiManager mMidiManager;
    private MidiDevice mMidiDevice;
    private MidiInputPort mInputPort;
    private Handler mHandler;
    private boolean mMidiEnabled = false;
    
    // Native callback method
    public static native void onMidiMessageReceived(byte[] data, int length);
    
    public JamulusMidiManager(Context context) {
        mContext = context;
        mHandler = new Handler(Looper.getMainLooper());
        
        // Check if MIDI is supported
        if (context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_MIDI)) {
            mMidiManager = (MidiManager) context.getSystemService(Context.MIDI_SERVICE);
            Log.i(TAG, "MIDI support detected");
        } else {
            Log.w(TAG, "MIDI not supported on this device");
        }
    }
    
    /**
     * Enable or disable MIDI input
     */
    public void setMidiEnabled(boolean enabled) {
        if (mMidiManager == null) {
            Log.w(TAG, "MIDI not supported - cannot enable");
            return;
        }
        
        if (enabled == mMidiEnabled) {
            return; // No change needed
        }
        
        if (enabled) {
            startMidiInput();
        } else {
            stopMidiInput();
        }
        
        mMidiEnabled = enabled;
        Log.i(TAG, "MIDI " + (enabled ? "enabled" : "disabled"));
    }
    
    /**
     * Check if MIDI is currently enabled
     */
    public boolean isMidiEnabled() {
        return mMidiEnabled && mMidiManager != null;
    }
    
    /**
     * Start MIDI input by opening the first available MIDI input device
     */
    private void startMidiInput() {
        if (mMidiManager == null) {
            return;
        }
        
        MidiDeviceInfo[] devices = mMidiManager.getDevices();
        
        // Find first device with input ports
        for (MidiDeviceInfo deviceInfo : devices) {
            if (deviceInfo.getInputPortCount() > 0) {
                openMidiDevice(deviceInfo);
                break;
            }
        }
        
        if (mMidiDevice == null) {
            Log.i(TAG, "No MIDI input devices found");
        }
    }
    
    /**
     * Open a specific MIDI device
     */
    private void openMidiDevice(MidiDeviceInfo deviceInfo) {
        mMidiManager.openDevice(deviceInfo, new MidiManager.OnDeviceOpenedListener() {
            @Override
            public void onDeviceOpened(MidiDevice device) {
                if (device == null) {
                    Log.e(TAG, "Failed to open MIDI device");
                    return;
                }
                
                mMidiDevice = device;
                
                // Open input port (port 0)
                mInputPort = device.openInputPort(0);
                if (mInputPort != null) {
                    // Set up MIDI receiver
                    mInputPort.connect(new MidiReceiver() {
                        @Override
                        public void onSend(byte[] data, int offset, int count, long timestamp) {
                            // Create a new array with just the MIDI data
                            byte[] midiData = new byte[count];
                            System.arraycopy(data, offset, midiData, 0, count);
                            
                            // Call native method to process MIDI data
                            onMidiMessageReceived(midiData, count);
                        }
                    });
                    
                    Log.i(TAG, "MIDI device opened: " + deviceInfo.getProperties().getString(MidiDeviceInfo.PROPERTY_NAME));
                } else {
                    Log.e(TAG, "Failed to open MIDI input port");
                    device.close();
                    mMidiDevice = null;
                }
            }
        }, mHandler);
    }
    
    /**
     * Stop MIDI input and close devices
     */
    private void stopMidiInput() {
        if (mInputPort != null) {
            try {
                mInputPort.close();
            } catch (IOException e) {
                Log.e(TAG, "Error closing MIDI input port", e);
            }
            mInputPort = null;
        }
        
        if (mMidiDevice != null) {
            try {
                mMidiDevice.close();
            } catch (IOException e) {
                Log.e(TAG, "Error closing MIDI device", e);
            }
            mMidiDevice = null;
        }
        
        Log.i(TAG, "MIDI input stopped");
    }
    
    /**
     * Get list of available MIDI devices
     */
    public List<String> getAvailableMidiDevices() {
        List<String> deviceNames = new ArrayList<>();
        
        if (mMidiManager == null) {
            return deviceNames;
        }
        
        MidiDeviceInfo[] devices = mMidiManager.getDevices();
        for (MidiDeviceInfo deviceInfo : devices) {
            if (deviceInfo.getInputPortCount() > 0) {
                String name = deviceInfo.getProperties().getString(MidiDeviceInfo.PROPERTY_NAME);
                if (name != null) {
                    deviceNames.add(name);
                } else {
                    deviceNames.add("MIDI Device " + deviceInfo.getId());
                }
            }
        }
        
        return deviceNames;
    }
    
    /**
     * Cleanup resources
     */
    public void cleanup() {
        stopMidiInput();
        mMidiManager = null;
        mContext = null;
        mHandler = null;
    }
}
