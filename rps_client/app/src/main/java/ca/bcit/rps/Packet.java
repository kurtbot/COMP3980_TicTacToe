package ca.bcit.rps;

import android.app.Activity;

public class Packet extends Activity {
    protected int type;
    protected int context;
    protected int length;
    protected int[] payload;

    // default constructor
    public Packet(){}

    public Packet(final int type, final int context, final int length,
                  final int[] payload){
        this.type = type;
        this.context = context;
        this.length = length;
        this.payload = payload;
    }

    public int[] getPayload(){
        return payload;
    }
}