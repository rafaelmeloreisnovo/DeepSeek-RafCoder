package com.rafcoder.app

import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {

    external fun nativeMessage(): String
    external fun nativeSectorReport(iterations: Int): String

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        val textView: TextView = findViewById(R.id.nativeText)
        textView.text = buildString {
            appendLine(nativeMessage())
            appendLine()
            append(nativeSectorReport(42))
        }
    }

    companion object {
        init {
            System.loadLibrary("rafcoder_native")
        }
    }
}
