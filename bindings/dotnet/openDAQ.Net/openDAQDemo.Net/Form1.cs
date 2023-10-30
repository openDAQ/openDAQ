/*
 * Copyright 2022-2023 Blueberry d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

using System.Media;

using Daq.Core.Objects;
using Daq.Core.OpenDAQ;
using Daq.Core.Types;

using Task = System.Threading.Tasks.Task; //ToDo: rename openDAQ Task to TaskObject


namespace openDAQDemo.Net;


public partial class Form1 : Form
{
    Instance _instance;

    public class DeviceDataSource
    {
        public string Name { get; set; }
        public string ConnectionString { get; set; }

        public DeviceDataSource(string name, string connectionString)
        {
            this.Name = name;
            this.ConnectionString = connectionString;
        }
    }

    public Form1()
    {
        InitializeComponent();
    }

    private void Form1_Load(object sender, EventArgs e)
    {
        this.btnConnect.Enabled    = false;
        this.btnDisconnect.Enabled = false;

        this.treeView1.Nodes.Clear();
        this.listBox1.Items.Clear();

        _instance = OpenDAQFactory.Instance();
    }

    private void Form1_FormClosing(object sender, FormClosingEventArgs e)
    {
        //_instance?.Dispose();
    }

    private void btnScan_Click(object sender, EventArgs e)
    {
        Cursor.Current     = Cursors.WaitCursor;
        this.Cursor        = Cursors.WaitCursor;
        this.UseWaitCursor = true;

        this.cmbDevices.BeginUpdate();

        this.cmbDevices.DataSource = null;

        TaskScheduler guiContext = TaskScheduler.FromCurrentSynchronizationContext();

        Task.Run(() => GetAvailableDevices())
            .ContinueWith(task => SetDataSource(task.Result), guiContext);

        this.cmbDevices.EndUpdate();

        this.UseWaitCursor = false;
        this.Cursor        = Cursors.Default;
        Cursor.Current     = Cursors.Default;
    }

    private void cmbDevices_SelectedIndexChanged(object sender, EventArgs e)
    {
        this.btnConnect.Enabled = !this.btnDisconnect.Enabled && (this.cmbDevices.SelectedItem != null);
    }

    private void btnConnect_Click(object sender, EventArgs e)
    {
        this.btnConnect.Enabled = false;

        try
        {
            var device = _instance.AddDevice((string)this.cmbDevices.SelectedValue);
            this.btnDisconnect.Enabled = true;

            FillTreeView(device);
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.Print("+++> exception thrown - {0}", ex.Message);
            SystemSounds.Exclamation.Play();
            this.btnConnect.Enabled = true;
        }
    }

    private void btnDisconnect_Click(object sender, EventArgs e)
    {
        this.treeView1.Nodes.Clear();
        this.listBox1.Items.Clear();

        try
        {
            Device device = _instance.GetDevices()[0];
            _instance.RemoveDevice(device);
            device.Dispose();
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.Print("+++> exception thrown - {0}", ex.Message);
        }

        this.btnConnect.Enabled = true;
        this.btnDisconnect.Enabled = false;
    }

    private void treeView1_AfterSelect(object sender, TreeViewEventArgs e)
    {
        this.listBox1.Items.Clear();

        if (e.Node?.Tag is Device device)
        {
            ListProperties(device);
        }
        else if (e.Node?.Tag is Signal signal)
        {
            ListProperties(signal);
        }
        else if (e.Node?.Tag is Channel channel)
        {
            ListProperties(channel);
        }
    }

    private List<DeviceDataSource> GetAvailableDevices()
    {
        var deviceInfos = _instance.GetAvailableDevices();

        var dataSource = new List<DeviceDataSource>();

        foreach (var deviceInfo in deviceInfos)
        {
            string connectionString = deviceInfo.GetConnectionString();

            if (!connectionString.StartsWith("daq"))
                continue;

            var deviceType = deviceInfo.GetDeviceType();

            dataSource.Add(new DeviceDataSource($"{deviceInfo.GetName()} ({deviceType.GetName()}) {deviceType.GetDescription()}", connectionString));
        }

        return dataSource;
    }

    private void SetDataSource(List<DeviceDataSource> dataSource)
    {
        this.cmbDevices.DataSource    = dataSource;
        this.cmbDevices.DisplayMember = "Name";
        this.cmbDevices.ValueMember   = "ConnectionString";
    }

    private void FillTreeView(Device device)
    {
        var node = this.treeView1.Nodes.Add(device.GetName());
        node.Tag = device;

        ListSignals(node.Nodes, device.GetSignals());
        ListChannels(node.Nodes, device.GetChannels());

        this.treeView1.ExpandAll();
    }

    private void ListSignals(TreeNodeCollection nodes, IListObject<Signal> signals)
    {
        foreach (var signal in signals)
        {
            var node = nodes.Add(signal.GetName());
            node.Tag = signal;
        }
    }

    private void ListChannels(TreeNodeCollection nodes, IListObject<Channel> channels)
    {
        foreach (var channel in channels)
        {
            var node = nodes.Add(channel.GetName());
            node.Tag = channel;

            ListSignals(node.Nodes, channel.GetSignals());
        }
    }

    private void ListProperties(PropertyObject propertyObject)
    {
        var properties = propertyObject.GetAllProperties();

        foreach (var property in properties)
        {
            string     propertyName        = property.GetName();
            CoreType   propertyType        = property.GetValueType();
            BaseObject propertyValueObject = propertyObject.GetPropertyValue(propertyName);
            BaseObject propertyValue       = CoreTypesFactory.GetPropertyValueObject(propertyValueObject, propertyType);

            this.listBox1.Items.Add($"{propertyName,-25} = {propertyValue}");
        }
    }
}
