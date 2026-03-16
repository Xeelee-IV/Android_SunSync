package com.example.sunsync

import android.annotation.SuppressLint
import java.util.Date
import java.util.Calendar
import java.text.SimpleDateFormat
import java.util.Locale

// This object acts as a namespace, similar to 'enum SunSync' in Swift
object SunSync {

    data class Device(
        val name: String,
        val address: String?,
        val uuid: String
    ) {
        val id: String get() = name
    }

    data class ColorRGB(
        var r: Int,
        var g: Int,
        var b: Int
    )

    data class Cycle(
        var start_color: ColorRGB,
        var start_brightness: Int,
        var end_color: ColorRGB,
        var end_brightness: Int,
        var start_time: Date,
        var duration: Int
    )

    data class Config(
        var uuid: String,
        var sleep: Cycle,
        var wake: Cycle
    )

    // Formatter for the "HH:mm" strings used in your JSON
    @SuppressLint("ConstantLocale")
    val timeFormatter = SimpleDateFormat("HH:mm", Locale.getDefault())
}