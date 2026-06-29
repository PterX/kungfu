import React from 'react';
import { createRoot } from 'react-dom/client';

// nodeIntegration exposes node `require` on window; use it to load the native
// kungfu binding at runtime from a vite-built (ESM) renderer. This is the moat:
// the renderer reaches the in-process runtime directly, no IPC copy.
declare global {
  interface Window {
    require: NodeRequire;
    process: NodeJS.Process;
  }
}

function loadBinding(): { ok: boolean; message: string; api: string[] } {
  try {
    const bindingPath = window.process.env.KFE_PATH;
    if (!bindingPath) {
      return { ok: false, message: 'KFE_PATH not set', api: [] };
    }
    const kfe = window.require(bindingPath);
    const api = Object.keys(kfe);
    return {
      ok: true,
      message: `loaded ${api.length} kungfu API`,
      api,
    };
  } catch (e) {
    return { ok: false, message: (e as Error).message, api: [] };
  }
}

function App() {
  const [state] = React.useState(loadBinding);

  return (
    <div
      style={{
        fontFamily: 'system-ui, sans-serif',
        color: '#ddd',
        background: '#1e1e1e',
        height: '100vh',
        margin: 0,
        padding: 24,
        boxSizing: 'border-box',
      }}
    >
      <h1 style={{ fontSize: 18, fontWeight: 600 }}>Kungfu reference app</h1>
      <p style={{ color: state.ok ? '#4ec9b0' : '#f48771' }}>
        {state.ok ? '● ' : '○ '}
        {state.message}
      </p>
      {state.api.length > 0 && (
        <ul style={{ columns: 3, fontSize: 13, color: '#9cdcfe' }}>
          {state.api.map((name) => (
            <li key={name}>{name}</li>
          ))}
        </ul>
      )}
    </div>
  );
}

createRoot(document.getElementById('root') as HTMLElement).render(<App />);
