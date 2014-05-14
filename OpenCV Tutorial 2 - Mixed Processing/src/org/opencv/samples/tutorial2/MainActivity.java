package org.opencv.samples.tutorial2;


import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;




public class MainActivity extends Activity  {
    

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) 
    {
       
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        
        setContentView(R.layout.activity_main);
        Button Start = (Button)findViewById(R.id.button1);
        Start.setOnClickListener(new Button.OnClickListener(){
        	@Override
        	public void onClick(View v){
        		
        		Intent intent = new Intent();
        		intent.setClass(MainActivity.this,Tutorial2Activity.class);
        		               
    
                startActivity(intent);
        	}
        });
        Button Exit = (Button)findViewById(R.id.button2);
        Exit.setOnClickListener(new Button.OnClickListener(){
        	@Override
        	public void onClick(View v){
        		
        		System.exit(0);
        	}
        });

    }

    
}
