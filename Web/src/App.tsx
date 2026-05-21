import React, { useState, useEffect } from 'react';
import { 
  CloudRain, 
  Droplets, 
  Thermometer, 
  Wind, 
  AlertTriangle,
  ToggleLeft,
  ToggleRight,
  Wifi,
  WifiOff,
  Activity,
  Settings,
  Leaf
} from 'lucide-react';
import {
  AreaChart,
  Area,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer
} from 'recharts';
import ChatBot from './ChatBot';

interface EdgeState {
  isConnected: boolean;
  airTemp: number;
  humidity: number;
  dewPoint: number;
  isValveOpen: boolean;
  isManualOverride: boolean;
}

export default function App() {
  const [state, setState] = useState<EdgeState>({
    isConnected: true,
    airTemp: 22.5,
    humidity: 65.0,
    dewPoint: 15.2,
    isValveOpen: false,
    isManualOverride: false
  });

  const [historyData, setHistoryData] = useState<any[]>([]);

  useEffect(() => {
    const fetchData = async () => {
      try {
        const response = await fetch('/api/web/data');
        if (!response.ok) throw new Error('Network error');
        const data = await response.json();
        
        if (data.telemetry_history && data.telemetry_history.length > 0) {
          const latest = data.telemetry_history[0];
          
          setState(s => ({
            ...s,
            airTemp: latest.hava_sicakligi || 0,
            humidity: latest.toprak_nemi || 0,
            dewPoint: latest.hava_sicakligi - ((100 - (latest.toprak_nemi || 0)) / 5), // rough dew point estimation
            isValveOpen: data.valve_status === 'OPEN',
            isConnected: true
          }));

          const chartData = [...data.telemetry_history].reverse().map((item: any) => ({
            time: new Date(item.olcum_zamani).toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'}),
            temp: item.hava_sicakligi,
            humidity: item.toprak_nemi
          }));
          setHistoryData(chartData);
        } else {
          // If no data yet, wait
          setState(s => ({ ...s, isConnected: true }));
        }
      } catch (error) {
        console.error('Failed to fetch telemetry:', error);
        setState(s => ({ ...s, isConnected: false }));
      }
    };

    fetchData();
    const interval = setInterval(fetchData, 5000);
    return () => clearInterval(interval);
  }, []);

  const isFrostWarning = state.airTemp <= 1.0;
  const isDewPointWarning = state.airTemp < state.dewPoint && state.airTemp < 2.0;

  const toggleOverride = async () => {
    const newValveState = !state.isValveOpen;
    const command = newValveState ? 'OPEN' : 'CLOSE';
    
    try {
      await fetch('/api/web/control', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ command }),
      });
      
      setState(s => ({
        ...s,
        isManualOverride: true,
        isValveOpen: newValveState
      }));
    } catch (error) {
      console.error('Error changing valve state:', error);
    }
  };

  const toggleConnection = () => {
    setState(s => ({ ...s, isConnected: !s.isConnected }));
  };

  return (
    <div className="min-h-screen bg-[#F8FAFC] text-slate-800 font-sans flex">
      {/* Sidebar Navigation */}
      <aside className="w-20 lg:w-64 bg-white border-r border-slate-200 hidden md:flex flex-col">
        <div className="h-20 flex items-center justify-center lg:justify-start lg:px-6 border-b border-slate-100">
          <Leaf className="w-8 h-8 text-emerald-500" />
          <span className="ml-3 font-bold text-xl text-slate-800 hidden lg:block tracking-tight">FarmEdge</span>
        </div>
        <nav className="flex-1 py-6 flex flex-col gap-2 px-3">
          <button className="flex items-center gap-3 px-3 py-3 rounded-xl bg-emerald-50 text-emerald-600 transition-colors">
            <Activity className="w-5 h-5" />
            <span className="font-medium hidden lg:block">Dashboard</span>
          </button>
          <button className="flex items-center gap-3 px-3 py-3 rounded-xl text-slate-500 hover:bg-slate-50 hover:text-slate-700 transition-colors">
            <Settings className="w-5 h-5" />
            <span className="font-medium hidden lg:block">Settings</span>
          </button>
        </nav>
      </aside>

      {/* Main Content */}
      <main className="flex-1 flex flex-col max-h-screen overflow-y-auto">
        {/* Header */}
        <header className="h-20 bg-white/50 backdrop-blur-md sticky top-0 z-10 border-b border-slate-200 px-6 lg:px-10 flex items-center justify-between">
          <div>
            <h1 className="text-2xl font-bold text-slate-800 tracking-tight">Overview</h1>
            <p className="text-sm text-slate-500 font-medium">Real-time telemetry and controls</p>
          </div>
          <button 
            onClick={toggleConnection}
            className={`flex items-center gap-2 px-4 py-2 rounded-full text-sm font-semibold transition-all shadow-sm ${state.isConnected ? 'bg-emerald-100/50 text-emerald-700 hover:bg-emerald-100' : 'bg-red-100/50 text-red-700 hover:bg-red-100'}`}
          >
            {state.isConnected ? <Wifi className="w-4 h-4" /> : <WifiOff className="w-4 h-4" />}
            <span className="hidden sm:inline">{state.isConnected ? 'Edge Online' : 'Edge Offline'}</span>
          </button>
        </header>

        <div className="p-6 lg:p-10 max-w-7xl mx-auto w-full space-y-8">
          
          {/* Metrics Grid */}
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-6">
            <MetricCard 
              icon={<Thermometer className="w-6 h-6 text-orange-500" />}
              title="Air Temperature"
              value={`${state.airTemp.toFixed(1)}°C`}
              warning={isFrostWarning ? 'Frost Warning Active' : null}
              statusColor="bg-orange-500"
            />
            <MetricCard 
              icon={<Wind className="w-6 h-6 text-sky-500" />}
              title="Air Humidity"
              value={`${state.humidity.toFixed(1)}%`}
              statusColor="bg-sky-500"
            />
            <MetricCard 
              icon={<Droplets className="w-6 h-6 text-indigo-500" />}
              title="Dew Point"
              value={`${state.dewPoint.toFixed(1)}°C`}
              warning={isDewPointWarning ? 'Dew Point Risk' : null}
              statusColor="bg-indigo-500"
            />
          </div>

          <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
            {/* Actuators & Controls */}
            <div className="bg-white rounded-2xl p-6 shadow-sm border border-slate-200">
              <div className="flex items-center justify-between mb-8">
                <h3 className="text-lg font-bold text-slate-800">Irrigation Control</h3>
                <div className={`px-2.5 py-1 text-xs font-bold rounded-md uppercase tracking-wider ${state.isManualOverride ? 'bg-amber-100 text-amber-700' : 'bg-slate-100 text-slate-500'}`}>
                  {state.isManualOverride ? 'Manual' : 'Auto'}
                </div>
              </div>
              
              <div className="bg-slate-50 rounded-xl p-5 border border-slate-100 mb-6">
                <div className="flex items-center justify-between">
                  <div>
                    <p className="font-semibold text-slate-800">Main Valve</p>
                    <p className="text-sm text-slate-500 mt-1">{state.isValveOpen ? 'Flowing (Watering)' : 'Closed (Standby)'}</p>
                  </div>
                  <button 
                    onClick={toggleOverride}
                    className={`transition-colors flex-shrink-0 ${state.isValveOpen ? 'text-emerald-500 hover:text-emerald-600' : 'text-slate-300 hover:text-slate-400'}`}
                  >
                    {state.isValveOpen ? <ToggleRight className="w-12 h-12" /> : <ToggleLeft className="w-12 h-12" />}
                  </button>
                </div>
              </div>

              {state.isManualOverride && (
                <button 
                  onClick={() => setState(s => ({ ...s, isManualOverride: false }))}
                  className="w-full py-3 px-4 bg-white border border-slate-200 hover:bg-slate-50 hover:border-slate-300 text-slate-700 font-semibold rounded-xl transition-all shadow-sm text-sm"
                >
                  Restore Automatic Control
                </button>
              )}
            </div>

            {/* Historical Chart */}
            <div className="bg-white rounded-2xl p-6 shadow-sm border border-slate-200 lg:col-span-2">
              <h3 className="text-lg font-bold text-slate-800 mb-6">Environment Trends (24h)</h3>
              <div className="h-64 mt-4">
                <ResponsiveContainer width="100%" height="100%">
                  <AreaChart data={historyData} margin={{ top: 0, right: 0, bottom: 0, left: -20 }}>
                    <defs>
                      <linearGradient id="colorTemp" x1="0" y1="0" x2="0" y2="1">
                        <stop offset="5%" stopColor="#f97316" stopOpacity={0.2}/>
                        <stop offset="95%" stopColor="#f97316" stopOpacity={0}/>
                      </linearGradient>
                      <linearGradient id="colorHum" x1="0" y1="0" x2="0" y2="1">
                        <stop offset="5%" stopColor="#0ea5e9" stopOpacity={0.2}/>
                        <stop offset="95%" stopColor="#0ea5e9" stopOpacity={0}/>
                      </linearGradient>
                    </defs>
                    <CartesianGrid strokeDasharray="3 3" vertical={false} stroke="#f1f5f9" />
                    <XAxis dataKey="time" axisLine={false} tickLine={false} tick={{ fontSize: 12, fill: '#94a3b8' }} dy={10} />
                    <YAxis yAxisId="left" axisLine={false} tickLine={false} tick={{ fontSize: 12, fill: '#94a3b8' }} />
                    <YAxis yAxisId="right" orientation="right" axisLine={false} tickLine={false} tick={{ fontSize: 12, fill: '#94a3b8' }} />
                    <Tooltip 
                      contentStyle={{ borderRadius: '12px', border: 'none', boxShadow: '0 10px 15px -3px rgb(0 0 0 / 0.1), 0 4px 6px -4px rgb(0 0 0 / 0.1)' }}
                      labelStyle={{ color: '#64748b', fontWeight: 600, marginBottom: '4px' }}
                    />
                    <Area yAxisId="left" type="monotone" dataKey="temp" stroke="#f97316" strokeWidth={3} fillOpacity={1} fill="url(#colorTemp)" name="Temp (°C)" />
                    <Area yAxisId="right" type="monotone" dataKey="humidity" stroke="#0ea5e9" strokeWidth={3} fillOpacity={1} fill="url(#colorHum)" name="Humidity (%)" />
                  </AreaChart>
                </ResponsiveContainer>
              </div>
            </div>
          </div>
        </div>
      </main>
      <ChatBot />
    </div>
  );
}

function MetricCard({ 
  icon, 
  title, 
  value, 
  warning,
  statusColor
}: { 
  icon: React.ReactNode; 
  title: string; 
  value: string; 
  warning?: string | null;
  statusColor: string;
}) {
  return (
    <div className="bg-white rounded-2xl p-6 shadow-sm border border-slate-200 relative overflow-hidden group hover:shadow-md transition-shadow">
      <div className={`absolute top-0 left-0 w-1 rounded-r-full h-full ${statusColor} opacity-50`} />
      <div className="flex justify-between items-start mb-4">
        <div className="p-3 bg-slate-50 rounded-xl border border-slate-100">
          {icon}
        </div>
        {warning && (
          <div className="flex items-center gap-1.5 px-2.5 py-1 bg-red-50 text-red-600 rounded-lg text-xs font-bold animate-pulse">
            <AlertTriangle className="w-3.5 h-3.5" />
            <span>Alert</span>
          </div>
        )}
      </div>
      <div>
        <p className="text-slate-500 text-sm font-semibold tracking-wide uppercase mb-1">{title}</p>
        <h2 className="text-3xl font-bold text-slate-800 tracking-tight">{value}</h2>
      </div>
      {warning && (
        <p className="text-red-500 text-xs font-medium mt-3 pt-3 border-t border-slate-100 leading-snug">
          Critial: {warning}
        </p>
      )}
    </div>
  )
}
