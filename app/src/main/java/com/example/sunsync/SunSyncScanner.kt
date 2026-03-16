package com.example.sunsync

import android.content.Context
import android.net.nsd.NsdManager
import android.net.nsd.NsdServiceInfo
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue

class SunSyncScanner(context: Context) {
    // Properties: foundDevices and isScanning
    val foundDevices = mutableStateListOf<SunSync.Device>()
    var isScanning by mutableStateOf(false)

    private val nsdManager = context.getSystemService(Context.NSD_SERVICE) as NsdManager

    // This listener reacts when a device is found on the network
    private val discoveryListener = object : NsdManager.DiscoveryListener {
        override fun onDiscoveryStarted(regType: String) {
            isScanning = true
        }

        override fun onServiceFound(serviceInfo: NsdServiceInfo) {
            // Android finds the service, but we must "resolve" it to get the IP address
            nsdManager.resolveService(serviceInfo, object : NsdManager.ResolveListener {
                override fun onResolveFailed(serviceInfo: NsdServiceInfo, errorCode: Int) {}

                override fun onServiceResolved(resolvedInfo: NsdServiceInfo) {
                    val host = resolvedInfo.host.hostAddress
                    val port = resolvedInfo.port
                    val address = "http://$host:$port"

                    val device = SunSync.Device(
                        name = resolvedInfo.serviceName,
                        address = address,
                        uuid = "" // I will update this via API call later
                    )

                    if (foundDevices.none { it.name == device.name }) {
                        foundDevices.add(device)
                    }
                }
            })
        }

        override fun onServiceLost(serviceInfo: NsdServiceInfo) {
            foundDevices.removeAll { it.name == serviceInfo.serviceName }
        }

        override fun onDiscoveryStopped(serviceInfo: String) {
            isScanning = false
        }

        override fun onStartDiscoveryFailed(serviceType: String, errorCode: Int) {
            nsdManager.stopServiceDiscovery(this)
        }

        override fun onStopDiscoveryFailed(serviceType: String, errorCode: Int) {
            nsdManager.stopServiceDiscovery(this)
        }
    }

    fun startScan() {
        // Scanning for "_http._tcp"
        nsdManager.discoverServices("_http._tcp.", NsdManager.PROTOCOL_DNS_SD, discoveryListener)
    }

    fun stopScan() {
        nsdManager.stopServiceDiscovery(discoveryListener)
    }
}