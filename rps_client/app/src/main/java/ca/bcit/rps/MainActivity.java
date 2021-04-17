package ca.bcit.rps;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    Button btnRock, btnPaper, btnScissors, btnStart;
    TextView p1Choice, p2Choice;

    int myChoice = 0;

    // As per protocol
    private static final int ROCK = 1;
    private static final int PAPER = 2;
    private static final int SCISSORS = 3;

    private static final int WIN = 1;
    private static final int LOSS = 2;
    private static final int TIE = 3;

    private Socket socket;
    private OutputStream toServer;
    private InputStream fromServer;

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
                // TODO: Replace with image?
                p1Choice.setText("ROCK");
                myChoice = 1;

                playerChoice(1, "ROCK");
            }
        });

        // Paper button
        btnPaper.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick (View v){
                // TODO: Replace with image?
                p1Choice.setText("PAPER");
                myChoice = 2;

                playerChoice(2, "PAPER");
            }
        });

        // Scissors button
        btnScissors.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick (View v){
                // TODO: Replace with image?
                p1Choice.setText("SCISSORS");
                myChoice = 3;

                playerChoice(3, "SCISSORS");
            }
        });

        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                Looper.prepare();
                connect();
                init();
//                initServerResp();
                sendMove();
            }
        };

        Thread thread = new Thread(runnable);
        thread.start();
    }

    private void connect() {
        Log.d("CONNECT", "Attempting to connect to " + Environment.HOST
                + ":" + Environment.PORT);
        try{
            socket = new Socket(Environment.HOST, Environment.PORT);
            DataOutputStream outputStream = new DataOutputStream(socket.getOutputStream());
            System.out.println("Output Stream: " + outputStream);

            Log.d("CONNECT", "Successfully connected");
        } catch (UnknownHostException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void init(){
        final int uid = 0;  // Initially 0 until server response
//        Log.d("HANDSHAKE", String.valueOf(DataTypes.RequestType.CONFIRMATION.getVal()));
        final int CONFIRMATION = DataTypes.RequestType.CONFIRMATION.getVal();
        final int CONTEXT = 1;
        final int length = 2;
        final int ver = 1;
        final int gId = 2;

        final int[] payload = {ver, gId};

        ReqPacket req = new ReqPacket(uid, CONFIRMATION, CONTEXT, length, payload);
        final byte[] packet = req.toByteArray();

        Log.d("INIT", req.toString());
        Log.d("INIT", packet.toString());

//        try {
//            toServer.write(packet);
//        } catch (IOException e) {
//            e.printStackTrace();
//        }
    }

    private void initServerResp() {
        byte[] packet = new byte[7];

//            int bytesRead = fromServer.read(packet);
            Log.d("PACKET", String.valueOf(packet));

//            if (bytesRead == 0){
//                Log.e("ERROR", "No bytes received from server");
//            }

            ByteBuffer bb = ByteBuffer.wrap(packet);

            int status = bb.get(0);
            int type = bb.get(1);
            int length = bb.get(2);
            int uid = bb.getInt(3);

            // bb.get(2) is 2nd index = length
            int[] recvPayload = new int[length];
            recvPayload[1] = uid;  // uid

            Packet recvPacket = new Packet(status, type, length, recvPayload);
            Player player = new Player(recvPacket.getPayload()[0]);

            Log.d("PLAYER", "Player ID: + " + player.getUid());

    }

    private void sendMove(){
        final int GAME_ACTION = DataTypes.RequestType.GAME_ACTION.getVal();
        final int CONTEXT = DataTypes.RequestContext.MAKE_MOVE.getVal();
        final int LENGTH = 1;

        Log.d("CHOICE", String.valueOf(myChoice));
    }

    private void playerChoice(final int choiceInt, final String choiceStr){
        Log.d("CHOICE", choiceStr);
    }
}