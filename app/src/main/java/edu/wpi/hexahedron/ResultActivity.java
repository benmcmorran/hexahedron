package edu.wpi.hexahedron;

import android.content.Intent;
import android.os.Bundle;
import android.app.Activity;
import android.text.method.ScrollingMovementMethod;
import android.view.View;
import android.widget.Button;
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
        view.setMovementMethod(new ScrollingMovementMethod());


        final Button button = (Button) findViewById(R.id.back);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                buttonClickFunction(v);
            }
        });
    }

    public void buttonClickFunction(View v)
    {
        Intent intent2 = new Intent(this, MainActivity.class);
        startActivity(intent2);
    }

}
