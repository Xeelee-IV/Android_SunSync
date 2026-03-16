package com.example.sunsync

import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.example.sunsync.ui.theme.SunSyncTheme
import androidx.compose.ui.tooling.preview.Preview

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val scanner = SunSyncScanner(this)

        setContent {
            SunSyncTheme {
                // 1. Create the Controller (The "Router")
                val navController = rememberNavController()

                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = Color.Black
                ) {
                    // 2. Define the Navigation Map
                    NavHost(navController = navController, startDestination = "scanning") {

                        // Route 1: The Scanner List
                        composable("scanning") {
                            ScanningView(
                                devices = scanner.foundDevices,
                                isScanning = scanner.isScanning,
                                onDeviceSelected = { device ->
                                    // When a device is clicked, navigate to the next screen
                                    // Passing the name as a simple identifier
                                    navController.navigate("device/${device.name}")
                                }
                            )
                        }

                        // Route 2: The Settings View
                        composable("device/{deviceName}") { backStackEntry ->
                            val name = backStackEntry.arguments?.getString("deviceName")
                            // Find the device from our list based on the name
                            val device = scanner.foundDevices.find { it.name == name }

                            if (device != null) {
                                DeviceView(device = device)
                            }
                        }
                    }
                }

                // Start scanning when app opens
                LaunchedEffect(Unit) { scanner.startScan() }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ScanningView(
    devices: List<SunSync.Device>,
    isScanning: Boolean,
    onDeviceSelected: (SunSync.Device) -> Unit
) {
    Scaffold(
        containerColor = Color.Black,
        topBar = {
            CenterAlignedTopAppBar(
                title = { Text("SunSync", color = Color.White) },
                colors = TopAppBarDefaults.centerAlignedTopAppBarColors(
                    containerColor = Color.Black,
                    titleContentColor = Color.White
                )
            )
        }
    ) { padding ->
        LazyColumn(modifier = Modifier.padding(padding)) {
            // Header showing status (Scanning or List Title)
            item {
                Text(
                    text = if (isScanning) "Scanning..." else "SunSync Devices",
                    modifier = Modifier.padding(16.dp),
                    color = Color.White
                )
            }

            // The clickable list items
            items(devices) { device ->
                ListItem(
                    headlineContent = { Text(device.name, color = Color.White) },
                    colors = ListItemDefaults.colors(
                        containerColor = Color.Black,
                        headlineColor = Color.White
                    ),
                    modifier = Modifier.clickable { onDeviceSelected(device) }
                )
            }
        }
    }
}



@Preview(showBackground = true)
@Composable
fun ScanningViewPreview() {
    SunSyncTheme {
        // A fake list of devices for the preview
        // All 3 required parameters: name, address, uuid
        val mockDevices = listOf(
            SunSync.Device("Mock SunSync Lamp", "00:11:22:33:44:55", "uuid-1")
        )

        ScanningView(
            devices = mockDevices,
            isScanning = false,
            onDeviceSelected = {}
        )
    }
}
