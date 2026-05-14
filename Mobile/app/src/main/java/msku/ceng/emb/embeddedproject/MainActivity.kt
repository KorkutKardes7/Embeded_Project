package msku.ceng.emb.embeddedproject

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import msku.ceng.emb.embeddedproject.ui.screens.DashboardScreen
import msku.ceng.emb.embeddedproject.ui.theme.EmbeddedProjectTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            EmbeddedProjectTheme {
                DashboardScreen()
            }
        }
    }
}
