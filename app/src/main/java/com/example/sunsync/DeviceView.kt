package com.example.sunsync

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.launch
import androidx.compose.ui.tooling.preview.Preview
import com.example.sunsync.ui.theme.SunSyncTheme
import java.util.Date

@Composable
fun DeviceView(device: SunSync.Device) {
    val controller = remember { SunSyncController(device) }
    var config by remember { mutableStateOf<SunSync.Config?>(null) }
    val scope = rememberCoroutineScope()

    LaunchedEffect(device) {
        config = controller.getConfig()
    }

    DeviceViewContent(
        config = config,
        onSave = {
            scope.launch {
                config?.let { controller.setConfig(it) }
            }
        }
    )
}

@Composable
fun DeviceViewContent(
    config: SunSync.Config?,
    onSave: () -> Unit
) {
    Surface(
        modifier = Modifier.fillMaxSize(),
        color = Color.Black,
        contentColor = Color.White
    ) {
        if (config != null) {
            DeviceSettings(
                config = config,
                onSave = onSave
            )
        } else {
            Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                CircularProgressIndicator(color = Color.White)
            }
        }
    }
}

@Composable
fun DeviceSettings(config: SunSync.Config, onSave: () -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        // Gap at the top for status bar/camera
        Spacer(modifier = Modifier.height(50.dp))

        Text(
            text = "Device: ${config.uuid}",
            style = MaterialTheme.typography.headlineSmall
        )

        // The weight(1f) here pushes everything below this LazyColumn to the bottom
        LazyColumn(modifier = Modifier.weight(1f)) {
            item { CycleSettings(title = "Sleep Cycle", cycle = config.sleep) }
            item { CycleSettings(title = "Wake Cycle", cycle = config.wake) }
        }

        Button(
            onClick = onSave,
            colors = ButtonDefaults.buttonColors(
                containerColor = Color.White,
                contentColor = Color.Black
            ),
            modifier = Modifier
                .fillMaxWidth()
                .padding(vertical = 16.dp)
                .navigationBarsPadding()
        ) {
            Text("Save Configuration")
        }
    }
}

@Composable
fun CycleSettings(title: String, cycle: SunSync.Cycle) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 8.dp),
        colors = CardDefaults.cardColors(
            containerColor = Color(0xFF1E1E1E), // Dark gray
            contentColor = Color.White
        )
    ) {
        Column(modifier = Modifier.padding(16.dp)) {
            var duration by remember { mutableStateOf(cycle.duration.toFloat()) }

            Text(title, style = MaterialTheme.typography.titleMedium)

            Text("Duration: ${duration.toInt()} min", color = Color.LightGray)
            Slider(
                value = duration,
                onValueChange = {
                    duration = it
                    cycle.duration = it.toInt()
                },
                valueRange = 1f..120f,
                colors = SliderDefaults.colors(
                    thumbColor = Color.White,
                    activeTrackColor = Color.White
                )
            )

            ColorControls(label = "Start Color", color = cycle.start_color)
        }
    }
}

@Composable
fun ColorControls(label: String, color: SunSync.ColorRGB) {
    Spacer(modifier = Modifier.height(8.dp))
    Text(label, style = MaterialTheme.typography.labelLarge, color = Color.LightGray)
    RGBRow("R", color.r, Color.Red) { color.r = it }
    RGBRow("G", color.g, Color.Green) { color.g = it }
    RGBRow("B", color.b, Color.Blue) { color.b = it }
}

@Composable
fun RGBRow(label: String, value: Int, accentColor: Color, onValueChange: (Int) -> Unit) {
    var currentValue by remember { mutableStateOf(value.toFloat()) }
    Row(verticalAlignment = Alignment.CenterVertically) {
        Text(label, modifier = Modifier.width(20.dp), color = accentColor)
        Slider(
            value = currentValue,
            onValueChange = {
                currentValue = it
                onValueChange(it.toInt())
            },
            valueRange = 0f..255f,
            modifier = Modifier.weight(1f),
            colors = SliderDefaults.colors(
                thumbColor = accentColor,
                activeTrackColor = accentColor
            )
        )
        Text(
            text = currentValue.toInt().toString(),
            modifier = Modifier.width(30.dp),
            style = MaterialTheme.typography.bodySmall,
            color = Color.LightGray
        )
    }
}

@Preview(showBackground = true)
@Composable
fun DeviceViewPreview() {
    val mockConfig = SunSync.Config(
        uuid = "mock-1234-uuid",
        sleep = SunSync.Cycle(
            start_color = SunSync.ColorRGB(255, 100, 0),
            start_brightness = 80,
            end_color = SunSync.ColorRGB(0, 0, 0),
            end_brightness = 0,
            start_time = Date(),
            duration = 30
        ),
        wake = SunSync.Cycle(
            start_color = SunSync.ColorRGB(0, 0, 0),
            start_brightness = 0,
            end_color = SunSync.ColorRGB(255, 255, 200),
            end_brightness = 100,
            start_time = Date(),
            duration = 20
        )
    )
    SunSyncTheme {
        DeviceViewContent(
            config = mockConfig,
            onSave = {}
        )
    }
}
