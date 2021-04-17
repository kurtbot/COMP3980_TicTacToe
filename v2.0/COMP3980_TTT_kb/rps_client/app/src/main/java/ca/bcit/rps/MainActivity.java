package ca.bcit.rps;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    Button btnRock, btnPaper, btnScissors;
    TextView p1Choice, p2Choice;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        btnRock = findViewById(R.id.btnRock);
        btnPaper = findViewById(R.id.btnPaper);
        btnScissors = findViewById(R.id.btnScissors);

        p1Choice = findViewById(R.id.p1Choice);
        p2Choice = findViewById(R.id.p2Choice);

        // Rock button
        btnRock.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick (View v){
                p1Choice.setText("ROCK");
            }
        });

        // Paper button
        btnPaper.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick (View v){
                p1Choice.setText("PAPER");
            }
        });

        // Scissors button
        btnScissors.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick (View v){
                p1Choice.setText("SCISSORS");
            }
        });
    }
}