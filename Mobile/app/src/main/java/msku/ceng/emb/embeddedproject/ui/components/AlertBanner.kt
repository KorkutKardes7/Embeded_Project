package msku.ceng.emb.embeddedproject.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material.icons.filled.WaterDrop
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import msku.ceng.emb.embeddedproject.EdgeState
import msku.ceng.emb.embeddedproject.FarmhouseViewModel
import msku.ceng.emb.embeddedproject.ui.theme.DangerRed
import msku.ceng.emb.embeddedproject.ui.theme.FrostWarningOrange

@Composable
fun AlertBanner(
    state: EdgeState,
    viewModel: FarmhouseViewModel
) {
    val isFrostWarn = viewModel.isFrostWarning(state)
    val isDewWarn = viewModel.isDewPointWarning(state)
    val isDrowning = viewModel.isDrowningRisk(state)
    
    val bgColor: Color
    val message: String
    val icon = if (isDrowning) Icons.Default.Warning else Icons.Default.WaterDrop

    when {
        isDrowning -> {
            bgColor = DangerRed
            message = "SAFETY OVERRIDE: Soil moisture critical. Valves forced CLOSED."
        }
        isFrostWarn || isDewWarn -> {
            bgColor = FrostWarningOrange
            message = "Frost Alarm Active - Irrigation Started"
        }
        state.isValveOpen -> {
            bgColor = MaterialTheme.colorScheme.tertiary
            message = "Irrigation is currently active"
        }
        else -> return // No prominent alert to show
    }

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(bgColor)
            .padding(16.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Icon(
            imageVector = icon,
            contentDescription = null,
            tint = Color.White
        )
        Spacer(modifier = Modifier.width(12.dp))
        Text(
            text = message,
            color = Color.White,
            style = MaterialTheme.typography.titleMedium,
            fontWeight = FontWeight.Bold
        )
    }
}
