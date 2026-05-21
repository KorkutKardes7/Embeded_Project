import React, { useState, useRef, useEffect } from 'react';
import { MessageCircle, X, Send, Bot, User, Loader2 } from 'lucide-react';

export default function ChatBot() {
  const [isOpen, setIsOpen] = useState(false);
  const [messages, setMessages] = useState<{ role: 'user' | 'bot'; content: string }[]>([
    { role: 'bot', content: 'Merhaba! Ben FarmEdge asistanınızım. Size nasıl yardımcı olabilirim?' }
  ]);
  const [input, setInput] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const messagesEndRef = useRef<HTMLDivElement>(null);

  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  };

  useEffect(() => {
    if (isOpen) {
      scrollToBottom();
    }
  }, [messages, isOpen]);

  const handleSend = async () => {
    if (!input.trim()) return;

    const userMessage = input.trim();
    setMessages(prev => [...prev, { role: 'user', content: userMessage }]);
    setInput('');
    setIsLoading(true);

    try {
      const response = await fetch('/api/web/chat', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ user_message: userMessage }),
      });

      const data = await response.json();

      if (data.status === 'success') {
        setMessages(prev => [...prev, { role: 'bot', content: data.response }]);
      } else {
        setMessages(prev => [...prev, { role: 'bot', content: 'Üzgünüm, bir hata oluştu.' }]);
      }
    } catch (error) {
      console.error('Chat error:', error);
      setMessages(prev => [...prev, { role: 'bot', content: 'Sunucuya bağlanılamadı.' }]);
    } finally {
      setIsLoading(false);
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSend();
    }
  };

  return (
    <>
      {/* Floating Button Button */}
      <button
        onClick={() => setIsOpen(true)}
        className={`fixed bottom-6 right-6 p-4 bg-emerald-500 text-white rounded-full shadow-lg hover:bg-emerald-600 transition-transform ${isOpen ? 'scale-0' : 'scale-100'} z-50`}
      >
        <MessageCircle size={24} />
      </button>

      {/* Chat Window */}
      <div
        className={`fixed bottom-6 right-6 w-80 sm:w-96 bg-white rounded-2xl shadow-2xl flex flex-col transition-all duration-300 origin-bottom-right z-50 overflow-hidden border border-slate-200 ${isOpen ? 'scale-100 opacity-100' : 'scale-0 opacity-0 pointer-events-none'}`}
        style={{ height: '500px', maxHeight: 'calc(100vh - 48px)' }}
      >
        {/* Header */}
        <div className="bg-emerald-500 text-white p-4 flex items-center justify-between">
          <div className="flex items-center gap-2">
            <Bot size={20} />
            <h3 className="font-bold">FarmEdge Asistanı</h3>
          </div>
          <button
            onClick={() => setIsOpen(false)}
            className="text-emerald-100 hover:text-white transition-colors"
          >
            <X size={20} />
          </button>
        </div>

        {/* Messages List flex-1 */}
        <div className="flex-1 overflow-y-auto p-4 space-y-4 bg-slate-50">
          {messages.map((msg, idx) => (
            <div
              key={idx}
              className={`flex items-start gap-2 ${msg.role === 'user' ? 'flex-row-reverse' : 'flex-row'}`}
            >
              <div className={`p-2 rounded-lg flex-shrink-0 ${msg.role === 'user' ? 'bg-emerald-100 text-emerald-600' : 'bg-slate-200 text-slate-600'}`}>
                {msg.role === 'user' ? <User size={16} /> : <Bot size={16} />}
              </div>
              <div
                className={`p-3 rounded-2xl max-w-[75%] text-sm ${
                  msg.role === 'user'
                    ? 'bg-emerald-500 text-white rounded-tr-none'
                    : 'bg-white border border-slate-200 text-slate-800 rounded-tl-none'
                }`}
              >
                {/* Parse Markdown if needed, else simple newlines */}
                {msg.content.split('\n').map((line, i) => (
                  <React.Fragment key={i}>
                    {line}
                    <br />
                  </React.Fragment>
                ))}
              </div>
            </div>
          ))}
          {isLoading && (
            <div className="flex items-start gap-2">
              <div className="p-2 rounded-lg flex-shrink-0 bg-slate-200 text-slate-600">
                <Bot size={16} />
              </div>
              <div className="p-3 bg-white border border-slate-200 rounded-2xl rounded-tl-none flex items-center gap-2">
                <Loader2 size={16} className="animate-spin text-emerald-500" />
                <span className="text-sm text-slate-500">Yazıyor...</span>
              </div>
            </div>
          )}
          <div ref={messagesEndRef} />
        </div>

        {/* Input Area */}
        <div className="p-3 bg-white border-t border-slate-200">
          <div className="flex items-end gap-2 bg-slate-50 rounded-xl border border-slate-200 p-1 focus-within:border-emerald-500 transition-colors">
            <textarea
              value={input}
              onChange={(e) => setInput(e.target.value)}
              onKeyDown={handleKeyDown}
              placeholder="Mesajınızı yazın..."
              className="flex-1 bg-transparent p-2 text-sm outline-none resize-none max-h-32 min-h-[40px]"
              rows={1}
            />
            <button
              onClick={handleSend}
              disabled={isLoading || !input.trim()}
              className="p-2 text-emerald-500 hover:text-emerald-600 disabled:text-slate-300 transition-colors"
            >
              <Send size={20} />
            </button>
          </div>
        </div>
      </div>
    </>
  );
}