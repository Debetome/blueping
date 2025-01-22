package com.android.blueping;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;

public class BlueManager {
    private BluetoothSocket socket;
    private OutputStream outStream;
    private InputStream inStream;

    // UUID for serial port communication
    private static final UUID SPP_UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

    public boolean connect(String deviceAddress) {
        try {
            BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
            if (adapter == null || !adapter.isEnabled()) {
                System.err.println("Bluetooth is not enabled.");
                return false;
            }

            BluetoothDevice device = adapter.getRemoteDevice(deviceAddress);
            socket = device.createRfcommSocketToServiceRecord(SPP_UUID);
            socket.connect();

            outStream = socket.getOutputStream();
            inStream = socket.getInputStream();
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public void sendPing(byte[] data) throws Exception {
        if (outStream != null) {
            outStream.write(data);
            outStream.flush();
        }
    }

    public byte[] receiveResponse() throws Exception {
        if (inStream != null) {
            byte[] buffer = new byte[600];
            int bytesRead = inStream.read(buffer);
            if (bytesRead > 0) {
                byte[] response = new byte[bytesRead];
                System.arraycopy(buffer, 0, response, 0, bytesRead);
                return response;
            }
        }
        return null;
    }

    public void disconnect() {
        try {
            if (socket != null) socket.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
