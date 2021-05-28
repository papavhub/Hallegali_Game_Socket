package JavaSocketClient;

import java.awt.BorderLayout;
import java.awt.event.*;
import java.net.InetAddress;
import java.net.Socket;
import java.io.*;
import javax.swing.*;
 
public class MySocketClient extends JFrame implements Runnable, ActionListener {
   
    JTextArea ta;
    JScrollPane pane;
    JTextField tf;
    Socket s;
    BufferedReader br;
    PrintWriter pw;
   
    public MySocketClient(){
        setTitle("채팅 클라이언트");
        setDefaultCloseOperation(DISPOSE_ON_CLOSE); //현재창만 닫기
       
        ta = new JTextArea(); // ui 관련 코드
        ta.setEditable(false);
        pane = new JScrollPane(ta);
        add(pane);
        tf = new JTextField();
        add(tf, BorderLayout.SOUTH);
       
        tf.addActionListener(this); //엔터치면 이벤트 발생
       
        setSize(400,300);
        setVisible(true);
       
       
        //네트워크 코드
        try {
            //s = new Socket(InetAddress.getLocalHost(),5000);
            s = new Socket("192.168.122.179",5000);
            //입력
            InputStream is = s.getInputStream();
            InputStreamReader isr = new InputStreamReader(is);
            br = new BufferedReader(isr);
            
            
            //출력
            OutputStream os = s.getOutputStream();
            pw = new PrintWriter(os, true);
           
            
           
        } catch (Exception e) {
           
        }
        //쓰레드 코드
        Thread t = new Thread(this);
        t.start();
    }
    @Override
    public void actionPerformed(ActionEvent e) { //엔터 입력했을 경우
        String chat = tf.getText(); //tf로 부터 채팅 내용을 가져온다.
        pw.println(chat);     
        System.out.println("나 : " + chat); // 확인용 출력
        ta.append("나 : " + chat + "\n");/// 내가 입력한거 ui 추가
        
        tf.setText(""); //tf를 지운다.       
    }
 
    @Override
    public void run() {
        try {
            while(true){
        		String str= br.readLine(); //채팅 내용
                //str = str.trim();
        		String match = "[^\uAC00-\uD7A3xfe0-9a-zA-Z\\s:/']";
                str =str.replaceAll(match, "");
                //한글유니코드(\uAC00-\uD7A3), 숫자 0~9(0-9), 영어 소문자a~z(a-z), 대문자A~Z(A-Z), 공백(\s)
                System.out.println(str); // 확인용 출력
                ta.append(str+"\n"); // ui에 추가              
                
                //스크롤 맨 밑으로
                ta.setCaretPosition(ta.getText().length());
            }
        } catch (Exception e) {
           
        }
       
 
    }
   
    public static void main(String[] args) {
        new MySocketClient();
    }
 
}