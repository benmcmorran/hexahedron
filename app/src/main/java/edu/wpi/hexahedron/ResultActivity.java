package edu.wpi.hexahedron;

import android.content.Intent;
import android.os.Bundle;
import android.app.Activity;
import android.widget.TextView;

public class ResultActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_result);
        setTitle("Steps to solve your Rubik's Cube");

        TextView view = (TextView) findViewById(R.id.result);
        Intent intent = getIntent();
        view.setText(intent.getExtras().getString("solve"));
    }

}
