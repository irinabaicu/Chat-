using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Net.Sockets;

namespace Client
{
    public partial class Form1 : Form
    {
        TcpClient _client;
        byte[] _buffer = new byte[4096];
        
        public Form1()
        {
            InitializeComponent();

            _client = new TcpClient(); 

        }

        protected override void OnShown(EventArgs e)
        {
            base.OnShown(e);

            // Connect to the remote server. The IP address and port # could be
            // picked up from a settings file.
            _client.Connect("192.168.100.5", 54000);

            // Start reading the socket and receive any incoming messages
            _client.GetStream().BeginRead(_buffer,
                                            0,
                                            _buffer.Length,
                                            Server_MessageReceived,
                                            null);
        }


        private void Server_MessageReceived(IAsyncResult ar)
        {
            if (ar.IsCompleted)
            {
                //Receive message
                var bytesIn = _client.GetStream().EndRead(ar);
                if (bytesIn > 0)
                {
                    var temp = new byte[bytesIn];
                    Array.Copy(_buffer, 0, temp, 0, bytesIn);
                    var str = Encoding.ASCII.GetString(temp);
                    if (String.Compare(str,"@exit") == 0)
                        this.Close();
                    this.Invoke((MethodInvoker)delegate{
                        listBox1.Items.Add(str);
                        listBox1.SelectedIndex = listBox1.Items.Count - 1;
                    });
                   
                }
                Array.Clear(_buffer, 0, _buffer.Length);

                _client.GetStream().BeginRead(_buffer, 0, _buffer.Length, Server_MessageReceived, null);
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            var msg = Encoding.ASCII.GetBytes(textBox1.Text);
            _client.GetStream().Write(msg, 0, msg.Length);

            textBox1.Text = "";
            textBox1.Focus();
         

        }
    }
}
