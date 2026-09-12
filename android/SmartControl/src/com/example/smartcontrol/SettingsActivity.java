package com.example.smartcontrol;

import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.Socket;
import java.nio.charset.Charset;










import com.example.smartcontrol_view.AddAndSubView;
import com.example.smartcontrol_view.AddAndSubView.OnNumChangeListener;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

//设置界面，实现设置，并把设置好的各个变量发送到MainActivity界面中，
//由MainActivity的发送按钮发送信息

public class SettingsActivity extends Activity implements OnClickListener,OnCheckedChangeListener{
	
	private String final_manual="";
	private String final_auto = "";
	//10个Toggle
	private int m_1;
	private int m_2;
	private int m_3;
	private int m_4;
	private int m_5;
	
	private int a_1;
	private int a_2;
	private int a_3;
	private int a_4;
	private int a_5;
	
	
	AddAndSubView wenduAdd;
	AddAndSubView yuzhiAdd;
	AddAndSubView lightAdd;
	AddAndSubView senwenAdd;
	 AddAndSubView rongchaAdd;
	
	
	//端口号IP
	private String ip;
	private String port;
	//控件
	private LinearLayout wenduSetting ;
	private LinearLayout yuzhiSetting;
	private LinearLayout lightSetting;
	private LinearLayout sewenSetting;
	private LinearLayout rongchaSetting;
	
	private ToggleButton manual_1;
	private ToggleButton manual_2;
	private ToggleButton manual_3;
	private ToggleButton manual_4;
	private ToggleButton manual_5;
	
	private ToggleButton auto_1;
	private ToggleButton auto_2;
	private ToggleButton auto_3;
	private ToggleButton auto_4;
	private ToggleButton auto_5;
	
	
	
	
	//按钮
	private Button back_Btn;
	private Button save_Btn;
	
	//是否连接
	private boolean isConnected = false;
	private Socket mSocket = null;
		
	//打印输出流和读入流
	private PrintWriter printWriter; 
	
	
	private TextView serverState;
	
	private Handler mHandler = new Handler(){
		
		@Override
		public void handleMessage(Message msg) {
			
			switch (msg.what) {
			
			case 2:

				serverState.setText("服务器已连接");
				isConnected = true;
				break;
			}
			
		};
	};

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.settings);
		initsViews();

		//监听事件
		back_Btn.setOnClickListener(this);
		save_Btn.setOnClickListener(this);
		manual_1.setOnCheckedChangeListener(this);
		manual_2.setOnCheckedChangeListener(this);
		manual_3.setOnCheckedChangeListener(this);
		manual_4.setOnCheckedChangeListener(this);
		manual_5.setOnCheckedChangeListener(this);
		
		auto_1.setOnCheckedChangeListener(this);
		auto_2.setOnCheckedChangeListener(this);
		auto_3.setOnCheckedChangeListener(this);
		auto_4.setOnCheckedChangeListener(this);
		auto_5.setOnCheckedChangeListener(this);
		
		
		
		
		
		//接收来自上一个活动的数据
//		Intent intent = getIntent();
		Bundle bundle = this.getIntent().getExtras();
		ip = bundle.getString("ip");
		port = bundle.getString("port");
		
		connectThread();
		
		
		
		
		
		
		
		
			
	}
	
	//初始化控件
	public void initsViews(){
		
		//初始化控件
		wenduSetting = (LinearLayout)findViewById(R.id.wendu_linearLayout);
		yuzhiSetting = (LinearLayout)findViewById(R.id.wenduyuzhi_linearLayout);
		lightSetting = (LinearLayout)findViewById(R.id.light_linearLayout);
		sewenSetting = (LinearLayout)findViewById(R.id.sewen_linearLayout);
		rongchaSetting = (LinearLayout)findViewById(R.id.rongcha_linearLayout);
		
		
		 wenduAdd = new AddAndSubView(this, 24);
		 yuzhiAdd = new AddAndSubView(this, 25);
		 lightAdd = new AddAndSubView(this, 2);
		 senwenAdd = new AddAndSubView(this, 2);
		 rongchaAdd = new AddAndSubView(this,5);
		
		wenduAdd.setOnNumChangeListener(new OnNumChangeListener() {
			
			@Override
			public void onNumChange(View view, int num) {
				// TODO Auto-generated method stub
				if(num<14 || num >32){
					Toast.makeText(getApplicationContext(), "请输入范围为14-32的数值", Toast.LENGTH_SHORT).show();
				}
			}
		});
		yuzhiAdd.setOnNumChangeListener(new OnNumChangeListener() {
			
			@Override
			public void onNumChange(View view, int num) {
				// TODO Auto-generated method stub
				if(num<20 || num >45){
					Toast.makeText(getApplicationContext(), "请输入范围为20-45的数值", Toast.LENGTH_SHORT).show();
				}
			}
		});
		lightAdd.setOnNumChangeListener(new OnNumChangeListener() {
			
			@Override
			public void onNumChange(View view, int num) {
				// TODO Auto-generated method stub
				if(num > 9){
					Toast.makeText(getApplicationContext(), "请输入范围为0-9的数值", Toast.LENGTH_SHORT).show();
				}
			}
		});
		senwenAdd.setOnNumChangeListener(new OnNumChangeListener() {
			
			@Override
			public void onNumChange(View view, int num) {
				// TODO Auto-generated method stub
				if(num >5){
					Toast.makeText(getApplicationContext(), "请输入范围为0-5的数值", Toast.LENGTH_SHORT).show();
				}
			}
		});
		rongchaAdd.setOnNumChangeListener(new OnNumChangeListener() {
			
			@Override
			public void onNumChange(View view, int num) {
				// TODO Auto-generated method stub
				if(num >10){
					Toast.makeText(getApplicationContext(), "请输入范围为0-10的数值", Toast.LENGTH_SHORT).show();
				}
			}
		});
		
//		Log.e("温度", wenduAdd.getNum()+"");getNum()可获取值
		
		wenduSetting.addView(wenduAdd);
		yuzhiSetting.addView(yuzhiAdd);
		lightSetting.addView(lightAdd);
		sewenSetting.addView(senwenAdd);
		rongchaSetting.addView(rongchaAdd);
		
		
		//按钮
		back_Btn = (Button)findViewById(R.id.settings_back_btn);
	
		save_Btn = (Button)findViewById(R.id.save_settings_btn);
		
		//textView
		
		serverState = (TextView)findViewById(R.id.setting_server_text);
		
		//toggleButton
		manual_1 = (ToggleButton)findViewById(R.id.manual_air_switch_btn_setting);
		manual_2 = (ToggleButton)findViewById(R.id.manual_clean_switch_btn_setting);
		manual_3 = (ToggleButton)findViewById(R.id.manual_exh_switch_btn_setting);
		manual_4 = (ToggleButton)findViewById(R.id.manual_window_switch_btn_setting);
		manual_5 = (ToggleButton)findViewById(R.id.manual_liht_switch_btn_setting);
		
		auto_1 = (ToggleButton)findViewById(R.id.auto_air_switch_btn_setting);
		auto_2 = (ToggleButton)findViewById(R.id.auto_clean_switch_btn_setting);
		auto_3 = (ToggleButton)findViewById(R.id.auto_exh_switch_btn_setting);
		auto_4 = (ToggleButton)findViewById(R.id.auto_window_switch_btn_setting);
		auto_5 = (ToggleButton)findViewById(R.id.auto_light_switch_btn_setting);
		
	}

	//监听
	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		switch (v.getId()) {
		case R.id.settings_back_btn:
			//设置界面
			Intent intent = new Intent(SettingsActivity.this,MainActivity.class);
			
			startActivity(intent);
			try {
				mSocket.close();
				
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			this.finish();
			break;
			

			
		case R.id.save_settings_btn:
			//保存设置和发送
			sendData();
			break;
		}
	}

	//发送
	private void sendData() {
		// TODO Auto-generated method stub
		try {
			
			final_manual = ""+m_1+m_2+m_3+m_4+m_5+"000";
			
			final_auto = ""+a_1+a_2+a_3+a_4+a_5+"000";
			
			String context = "{"
					
			+wenduAdd.getNum()+","
					+yuzhiAdd.getNum()+","
			+rongchaAdd.getNum()+","
					+lightAdd.getNum()+","
			+senwenAdd.getNum()+","
					+Integer.valueOf(final_manual,2)+","
			+Integer.valueOf(final_auto,2)+"}";
			
			Log.e("context", context);
			
			if (!context.equals("")) {

				if (printWriter == null) {
					showInfo("连接失败!");
					return;
				}
				
				printWriter.print(context);
				printWriter.flush();
				
			}else{
				showInfo("发送的数据不能为空");
			}

			
			
		} catch (Exception e) {
			// TODO: handle exception
			Log.e("发送失败", e.getMessage());
		}
	}

	//连接
	private void connectThread() {
		// TODO Auto-generated method stub
		if(!isConnected){
			//没连接就开启一个连接线程
			
			new Thread(new Runnable() {
				
				@Override
				public void run() {
					// TODO Auto-generated method stub
					Looper.prepare();
					connectServer(ip,port);	
				}
			}).start();
			
			
		}else{
			//再次监听可断开
			if(mSocket!=null){
				try {
					mSocket.close();
					mSocket =null;
					Log.e("mSocket", "断开连接");
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				
			}
			

			isConnected = false;
			
			
			
		}
	}
	
	//链接到服务器
		protected void connectServer(String ip, String port) {
			// TODO Auto-generated method stub
			try {
				
				mSocket = new Socket(ip,Integer.parseInt(port));
				
				OutputStream outputStream = mSocket.getOutputStream();
				
				//获取输出流，用打印流高效
				printWriter = new PrintWriter(new BufferedWriter(
						new OutputStreamWriter(outputStream,
								Charset.forName("gb2312"))));
				
				mHandler.sendEmptyMessage(2);
				
				
			} catch (Exception e) {
				// TODO Auto-generated catch block
				Toast.makeText(getApplicationContext(), "连接服务器失败", Toast.LENGTH_SHORT).show();
				isConnected = false;	
			}
			
			
		}
		
		
		private void showInfo(String msg) {
			Toast.makeText(SettingsActivity.this, msg, Toast.LENGTH_SHORT).show();

		}

		
		//toggleButton监听机制
		@Override
		public void onCheckedChanged(CompoundButton buttonView,
				boolean isChecked) {
			// TODO Auto-generated method stub
			switch (buttonView.getId()) {
			case R.id.manual_air_switch_btn_setting:
				if(isChecked){
					m_1 = 1;
					Log.e("m_1", m_1+"");
				}else{
					m_1 = 0;
					Log.e("m_1", m_1+"");
				}
				break;
			case R.id.manual_clean_switch_btn_setting:
				if(isChecked){
					m_2 = 1;
				}else{
					m_2 = 0;
				}
				break;
			case R.id.manual_exh_switch_btn_setting:
				if(isChecked){
					m_3 = 1;
				
				}else{
					m_3 = 0;
				}
				break;
			case R.id.manual_liht_switch_btn_setting:
				if(isChecked){
					m_4 = 1;
					
				}else{
					m_4 = 0;
				}
				break;
			case R.id.manual_window_switch_btn_setting:
				if(isChecked){
					m_5 = 1;
					
				}else{
					m_5 = 0;
				}
				
				break;
			case R.id.auto_air_switch_btn_setting:
				if(isChecked){
					a_1 = 1;
					
				}else{
					a_1 = 0;
				}
				
				break;
			case R.id.auto_clean_switch_btn_setting:
				
				if(isChecked){
					a_2 = 1;
					
				}else{
					a_2 = 0;
				}
				
				break;
			case R.id.auto_exh_switch_btn_setting:
				if(isChecked){
					a_3 = 1;
					
				}else{
					a_3 = 0;
				}
				
				break;
			case R.id.auto_light_switch_btn_setting:
				if(isChecked){
					a_4 = 1;
					
				}else{
					a_4 = 0;
				}
				break;
			case R.id.auto_window_switch_btn_setting:
				if(isChecked){
					a_5 = 1;
					
				}else{
					a_5 = 0;
				}
				break;

			
			}
			
		}

		
	
	
}
