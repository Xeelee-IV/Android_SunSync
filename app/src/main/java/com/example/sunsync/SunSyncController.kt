package com.example.sunsync

import kotlinx.serialization.json.Json
import kotlinx.serialization.encodeToString
import okhttp3.*
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.RequestBody.Companion.toRequestBody
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

class SunSyncController(private val device: SunSync.Device) {
    private val client = OkHttpClient()
    private val jsonInterpreter = Json { ignoreUnknownKeys = true }

    suspend fun getConfig(): SunSync.Config? = withContext(Dispatchers.IO) {
        val request = Request.Builder()
            .url("${device.address}/api/config")
            .build()

        try {
            client.newCall(request).execute().use { response ->
                if (!response.isSuccessful) return@withContext null
                val bodyString = response.body?.string() ?: return@withContext null
                // Use Kotlin Serialization to turn JSON string into our Data Class
                jsonInterpreter.decodeFromString<SunSync.Config>(bodyString)
            }
        } catch (e: Exception) {
            e.printStackTrace()
            null
        }
    }

    suspend fun setConfig(config: SunSync.Config): Boolean = withContext(Dispatchers.IO) {
        val mediaType = "application/json; charset=utf-8".toMediaType()
        val jsonString = jsonInterpreter.encodeToString(config)
        val body = jsonString.toRequestBody(mediaType)

        val request = Request.Builder()
            .url("${device.address}/api/config")
            .post(body)
            .build()

        try {
            client.newCall(request).execute().use { response ->
                response.isSuccessful
            }
        } catch (e: Exception) {
            false
        }
    }
}