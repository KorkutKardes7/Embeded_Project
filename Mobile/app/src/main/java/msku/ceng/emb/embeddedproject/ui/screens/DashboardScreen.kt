package msku.ceng.emb.embeddedproject.ui.screens

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.expandVertically
import androidx.compose.animation.shrinkVertically
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.GridItemSpan
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import msku.ceng.emb.embeddedproject.FarmhouseViewModel
import msku.ceng.emb.embeddedproject.ui.components.AlertBanner
import msku.ceng.emb.embeddedproject.ui.components.SensorCard
import msku.ceng.emb.embeddedproject.ui.theme.DangerRed

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DashboardScreen(viewModel: FarmhouseViewModel = viewModel()) {
    val uiState by viewModel.uiState.collectAsState()
    val isAlertActive = viewModel.isFrostWarning(uiState) || viewModel.isDewPointWarning(uiState) || viewModel.isDrowningRisk(uiState) || uiState.isValveOpen

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Automated Frost Prevention") },
                actions = {
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        modifier = Modifier.padding(end = 8.dp)
                    ) {
                        Icon(
                            imageVector = Icons.Default.Circle,
                            contentDescription = "Connected",
                            tint = if (uiState.isConnected) MaterialTheme.colorScheme.primary else DangerRed,
                            modifier = Modifier.size(12.dp)
                        )
                        Spacer(modifier = Modifier.width(4.dp))
                        Text(
                            text = if (uiState.isConnected) "Edge Connected" else "Disconnected",
                            style = MaterialTheme.typography.labelSmall
                        )
                        Spacer(modifier = Modifier.width(16.dp))
                        IconButton(onClick = { /* TODO: Pairing Action */ }) {
                            Icon(Icons.Default.Wifi, contentDescription = "Connect via Wi-Fi")
                        }
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.surface,
                    titleContentColor = MaterialTheme.colorScheme.onSurface
                )
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
        ) {
            AnimatedVisibility(
                visible = isAlertActive,
                enter = expandVertically(),
                exit = shrinkVertically()
            ) {
                AlertBanner(state = uiState, viewModel = viewModel)
            }

            LazyVerticalGrid(
                columns = GridCells.Fixed(2),
                contentPadding = PaddingValues(16.dp),
                horizontalArrangement = Arrangement.spacedBy(16.dp),
                verticalArrangement = Arrangement.spacedBy(16.dp),
                modifier = Modifier.fillMaxSize()
            ) {
                // Valve Status Card (Critical - spans both columns)
                item(span = { GridItemSpan(2) }) {
                    Card(
                        colors = CardDefaults.cardColors(
                            containerColor = MaterialTheme.colorScheme.surfaceVariant
                        ),
                        modifier = Modifier.fillMaxWidth()
                    ) {
                        Row(
                            modifier = Modifier
                                .padding(16.dp)
                                .fillMaxWidth(),
                            horizontalArrangement = Arrangement.SpaceBetween,
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Column {
                                Text(
                                    "Valve Node",
                                    style = MaterialTheme.typography.labelMedium
                                )
                                Text(
                                    text = if (uiState.isValveOpen) "OPEN (WATERING)" else "CLOSED",
                                    style = MaterialTheme.typography.titleLarge,
                                    fontWeight = FontWeight.Bold,
                                    color = if (uiState.isValveOpen) MaterialTheme.colorScheme.tertiary else MaterialTheme.colorScheme.onSurface
                                )
                            }
                            Switch(
                                checked = uiState.isValveOpen,
                                onCheckedChange = { viewModel.toggleManualOverride(it) }
                            )
                        }
                    }
                }

                item(span = { GridItemSpan(2) }) {
                    Text(
                        "Sensor Telemetry",
                        style = MaterialTheme.typography.titleMedium,
                        modifier = Modifier.padding(top = 8.dp, bottom = 4.dp)
                    )
                }

                // Sensors Grid
                item {
                    SensorCard(
                        title = "Ambient Air Temp",
                        value = String.format("%.1f", uiState.airTemp),
                        unit = "°C",
                        icon = Icons.Default.Thermostat,
                        isHighlighted = true // Ground truth highlighted
                    )
                }
                
                item {
                    SensorCard(
                        title = "Humidity",
                        value = String.format("%.1f", uiState.humidity),
                        unit = "%",
                        icon = Icons.Default.WaterDrop
                    )
                }
                
                item {
                    SensorCard(
                        title = "Dew Point",
                        value = String.format("%.1f", uiState.dewPoint),
                        unit = "°C",
                        icon = Icons.Default.AcUnit
                    )
                }
                
                item {
                    SensorCard(
                        title = "Soil Moisture",
                        value = String.format("%.1f", uiState.soilMoisture),
                        unit = "%",
                        icon = Icons.Default.Grass
                    )
                }
                
                item(span = { GridItemSpan(2) }) {
                    Spacer(modifier = Modifier.height(24.dp))
                }
            }
        }
    }
}
