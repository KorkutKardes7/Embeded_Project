package msku.ceng.emb.embeddedproject

import androidx.lifecycle.ViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update

// TODO: Map these fields to actual sensor payload. Current defaults are DUMMY values for UI preview.
data class EdgeState(
    val isConnected: Boolean = true,
    val airTemp: Double = 0.5, // TODO: Replace dummy value with real DHT22 telemetry
    val humidity: Double = 85.0, // TODO: Replace dummy value with real DHT22 telemetry
    val dewPoint: Double = -1.0, // TODO: Replace dummy value with calculated telemetry
    val soilMoisture: Double = 88.0, // TODO: Replace dummy value with real soil sensor telemetry
    val isValveOpen: Boolean = true, // Valve open due to warning
    val isManualOverride: Boolean = false
)

class FarmhouseViewModel : ViewModel() {
    // TODO: Initialize StateFlow with actual edge device state instead of dummy EdgeState()
    private val _uiState = MutableStateFlow(EdgeState())
    val uiState: StateFlow<EdgeState> = _uiState.asStateFlow()

    fun toggleManualOverride(isOpen: Boolean) {
        _uiState.update { 
            it.copy(
                isManualOverride = true,
                isValveOpen = isOpen
            ) 
        }
    }
    
    // Compute warnings based on requirements
    fun isFrostWarning(state: EdgeState) = state.airTemp <= 1.0
    fun isDewPointWarning(state: EdgeState) = state.airTemp < state.dewPoint && state.airTemp < 2.0
    fun isDrowningRisk(state: EdgeState) = state.soilMoisture >= 95.0
}
