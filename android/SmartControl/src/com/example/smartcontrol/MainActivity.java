package com.example.smartcontrol;




import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintStream;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.charset.Charset;







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
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

//主界面实现连接，数据监控等功能

public class MainActivity extends Activity implements OnClickListener{
	
	private Button settingsBtn ;
	
	//各个显示控件
	private TextView p1;
	private TextView p2;
	private TextView p3;
	private TextView p4;
	private TextView p5;
	private TextView p6;
	private TextView p7;
	private TextView p8;
	private TextView p9;
	private TextView p10;
	private TextView p11;
	private TextView p12;
	private TextView p13;
	private TextView p14;
	private TextView p15;
	private TextView p16;
	
	
	
	private ImageView manual_1;
	private ImageView manual_2;
	private ImageView manual_3;
	private ImageView manual_4;
	private ImageView manual_5;
	
	
	private ImageView auto_1;
	private ImageView auto_2;
	private ImageView auto_3;
	private ImageView auto_4;
	private ImageView auto_5;
	
	
	//内定端口号IP
	private String  ip = "119.29.201.135";
	private String  port ="8001";
	
	//是否连接
	private boolean isConnected = false;
	private Socket mSocket = null;
	
	//打印输出流和读入流
	private InputStream in;
	private PrintWriter printWriter; 
	
	//接收线程
	private Thread reciverThread;
	

	private TextView serverState;
	
	//UI线程处理
	private Handler mHandler = new Handler(){
		
		@Override
		public void handleMessage(Message msg) {
			
			switch (msg.what) {
			
			case 1:
				//接收处理，str为接收的数据显示
				String str = (String) msg.obj;
				Log.e("str", str);
				//解析数据
				if(str.contains("{")){
					phrasestr(str);
				}
				
				break;
				
			case 2:
				//开启接收数据线程
				reciverThread = new Thread(new MyReceiverRunnale());
				reciverThread.start();
			
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
		setContentView(R.layout.main_activity);
		initsViews();
		
		connectThread();
		settingsBtn.setOnClickListener(this);
	
	}
	
	//解析
	protected void phrasestr(String str) {
		// TODO Auto-generated method stub
		

		String st = str.substring(1,str.length()-1);
		Log.e("st", st);
		
		
		String[] s = st.split(",");
	
//		//解析第1个数据
		p1.setText(s[0]);
		//解析第2个数据
		p2.setText(s[1]);
		//解析第3个数据
		p3.setText(s[2]);
		//解析第4个数据
		p4.setText(s[3]);
		//解析第5个数据
		p5.setText(s[4]);
		//解析第6个数据
		p6.setText(s[5]);
		
		//解析第7个数据
		p7phrase(s[6]);
		//解析第8个数据
		p8phrase(s[7]);
		//解析第9个数据
		p9phrase(s[8]);
		//解析第10个数据
		p10phrase(s[9]);
		//解析第11个数据
		p11phrase(s[10]);
		//解析第12个数据
		p12phrase(s[11]);
		//解析第13个数据
		p8.setText(s[12]);
		//解析第14个数据
		p16.setText(s[13]);
				
	}
	
	
	//用于补全8位
	private String getEightBit(String s){
		
		String st ="";
		int length =8-s.length();
		for(int i=0;i<length;i++){
			st+="0";
		}
		
		s = st+s;
		
		return s;
		
	}
	
	private void p11phrase(String s){
		char[] ch = s.toCharArray();
		if(ch[0]=='1'){
			p15.setText("开");
		}else{
			p15.setText("关");
		}
	}
	
	
	private void p10phrase(String s){
		char[] ch = s.toCharArray();
		if(ch[0]=='1'){
			p14.setText("开");
		}else{
			p14.setText("关");
		}
	}
	
	
	private void p12phrase(String s) {
		// TODO Auto-generated method stub
		String st1 = Integer.toBinaryString(Integer.valueOf(s));
		
		st1 = getEightBit(st1);
		Log.e("12", st1);
		
		char[] st1Char = st1.toCharArray();
		
		if(st1Char[0]=='1'){
			p12.setText("开");
			
		}else{
			p12.setText("关");
		}
		
		String st2 = st1.substring(1, 4);
		Log.e("12", st2);
		p10.setText(Integer.valueOf(st2,2)+"");
		String st3 = st1.substring(4);
		Log.e("12", st3);
		p9.setText(Integer.valueOf(st3, 2)+"");
		
	}

	public void p7phrase(String s){
		
		String st1 = Integer.toBinaryString(Integer.valueOf(s));
		
		
		st1 = getEightBit(st1);
		Log.e("7", st1);
		
		char[] st1Char = st1.toCharArray();
		
		//空调
		if(st1Char[0]=='1'){
			auto_1.setImageResource(R.drawable.on1);
		}else{
			auto_1.setImageResource(R.drawable.off1);
		}
		
		if(st1Char[1]=='1'){
			auto_2.setImageResource(R.drawable.on1);
		}else{
			auto_2.setImageResource(R.drawable.off1);
		}
		
		if(st1Char[2]=='1'){
			auto_3.setImageResource(R.drawable.on1);
		}else{
			auto_3.setImageResource(R.drawable.off1);
		}
		
		if(st1Char[3]=='1'){
			auto_4.setImageResource(R.drawable.on1);
		}else{
			auto_4.setImageResource(R.drawable.off1);
		}
		
		if(st1Char[4]=='1'){
			auto_5.setImageResource(R.drawable.on1);
		}else{
			auto_5.setImageResource(R.drawable.off1);
		}
		
		if(st1Char[5]=='1'){
			p11.setText("开");
			
		}else{
			p11.setText("关");
		}
	}
	
	
	public void p8phrase(String s){
		
		String st1 = Integer.toBinaryString(Integer.valueOf(s));
		
		st1 = getEightBit(st1);
		
		Log.e("8", st1);
		
		char[] st1Char = st1.toCharArray();
		
		//空调
		if(st1Char[0]=='1'){
			manual_1.setImageResource(R.drawable.on1);
		}else{
			manual_1.setImageResource(R.drawable.off1);
		}
		
		if(st1Char[1]=='1'){
			manual_2.setImageResource(R.drawable.on1);
		}else{
			manual_2.setImageResource(R.drawable.off1);
		}
		
		if(st1Char[2]=='1'){
			manual_3.setImageResource(R.drawable.on1);
		}else{
			manual_3.setImageResource(R.drawable.off1);
		}
		
		if(st1Char[3]=='1'){
			manual_4.setImageResource(R.drawable.on1);
		}else{
			manual_4.setImageResource(R.drawable.off1);
		}
		
		if(st1Char[4]=='1'){
			manual_5.setImageResource(R.drawable.on1);
		}else{
			manual_5.setImageResource(R.drawable.off1);
		}
		
	}
	
	
	public void p9phrase(String s){
		String st1 = Integer.toBinaryString(Integer.valueOf(s));
		
		st1 = getEightBit(st1);
		
		char[] ch = st1.toCharArray();
		
		Log.e("9", st1);
		if(ch[0]=='1'){
			p13.setText("开");
		}else{
			p13.setText("关");
		}
		String st2 = st1.substring(1);
		Log.e("9", st2);
		p7.setText(Integer.valueOf(st2, 2)+"");
	}
	
	
	public void initsViews(){
		
		settingsBtn = (Button)findViewById(R.id.id_settings_btn);	
		serverState = (TextView)findViewById(R.id.id_servers_text);
		p1= (TextView)findViewById(R.id.id_pm2_5_text);
		p2= (TextView)findViewById(R.id.id_yanwu_text);
		p3= (TextView)findViewById(R.id.id_keranqiti_text);
		p4= (TextView)findViewById(R.id.id_pa_text);
		p5= (TextView)findViewById(R.id.id_shiwen_text);
		p6= (TextView)findViewById(R.id.id_shiwaiwen_text);
		p7= (TextView)findViewById(R.id.id_airwendu_text);
		//少了个容差
		p16 = (TextView)findViewById(R.id.id_rongcha_text);
		
		p8= (TextView)findViewById(R.id.id_yuzhi_text);
		p9= (TextView)findViewById(R.id.id_lamp_light_text);
		p10= (TextView)findViewById(R.id.id_lampsewen_text);
		p11= (TextView)findViewById(R.id.id_window_text);
		p12= (TextView)findViewById(R.id.id_light_text);
		p13= (TextView)findViewById(R.id.id_air_text);
		p14= (TextView)findViewById(R.id.id_clean_text);
		p15= (TextView)findViewById(R.id.id_exh_text);
		
		
		
		
		manual_1 = (ImageView)findViewById(R.id.manual_air_state);
		manual_2 = (ImageView)findViewById(R.id.manual_clean_state);
		manual_3 = (ImageView)findViewById(R.id.manual_exh_state);
		manual_4 = (ImageView)findViewById(R.id.manual_window_state);
		manual_5 = (ImageView)findViewById(R.id.manual_light_state);
		
		
		auto_1 = (ImageView)findViewById(R.id.auto_air_state);
		auto_2 = (ImageView)findViewById(R.id.auto_clean_state);
		auto_3 = (ImageView)findViewById(R.id.auto_exh_state);
		auto_4 = (ImageView)findViewById(R.id.auto_window_state);
		auto_5 = (ImageView)findViewById(R.id.auto_light_state);
			
	}
	
	
	


	//按钮监听
	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		switch (v.getId()) {
		case R.id.id_settings_btn:
			
			if(isConnected){
				
				Intent intent = new Intent(MainActivity.this,SettingsActivity.class);
				Bundle bundle = new Bundle();
				bundle.putString("ip", ip);
				bundle.putString("port", port);
				intent.putExtras(bundle);
				startActivity(intent);
				this.finish();
				
				try {
					mSocket.close();
					reciverThread.interrupt();

					serverState.setText("未连接服务器");
					
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}else{
				Toast.makeText(this, "请先连接服务器", Toast.LENGTH_SHORT).show();
			}
			//设置界面
			
			break;
		}
	}
	
	
	//发送数据
	private void sendData() {
		// TODO Auto-generated method stub
		
		try {
			
			String context = "hello world";
			
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


	//连接线程
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
			

			serverState.setText("未连接服务器");
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
			//listener.setOutStream(printWriter);监听是否要延迟
			
			//获取读入流
			in = mSocket.getInputStream();
			//发送到线程处理，打开接受数据线程
			mHandler.sendEmptyMessage(2);
			showInfo("连接成功");	
			
		} catch (Exception e) {
			// TODO Auto-generated catch block
			
			isConnected = false;
			Toast.makeText(getApplicationContext(), "连接服务器失败", Toast.LENGTH_SHORT).show();
			
			
			
		}
		
		
	}
	
	private class MyReceiverRunnale implements Runnable {

		public void run() {

			while (true) {
				
				if (isConnected) {
					if (mSocket != null && mSocket.isConnected()) {

						String result = "";
						//读取数据
						int count = 0;
						byte[] inDatas = null;
						try {
							while (count == 0) {
								count = in.available();
							}
							inDatas = new byte[count];
							in.read(inDatas);
							//服务器编码方式为gb2312,所以用这个编码方式接收
							result =  new String(inDatas, "gb2312");
							
							
							//发送UI线程处理
							if (!result.equals("")) {

								Message msg = new Message();
								msg.what = 1;
								msg.obj = result;
								mHandler.sendMessage(msg);
							}

						} catch (Exception e) {
							Log.e("e", e.getMessage());
							e.printStackTrace();
						}
					}
				}
				//延迟接收
				try {
					Thread.sleep(100L);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}

			}
		}
	}


	private void showInfo(String msg) {
		Toast.makeText(MainActivity.this, msg, Toast.LENGTH_SHORT).show();

	}

}
