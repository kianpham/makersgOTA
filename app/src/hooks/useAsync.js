import { useCallback, useEffect, useState } from 'react'

// Hook fetch: {loading, data, error, reload}. Tùy chọn pollMs để tự refresh (silent, không nháy skeleton).
export function useAsync(fn, deps = [], { pollMs } = {}) {
  const [state, setState] = useState({ loading: true, data: null, error: null })

  // eslint-disable-next-line react-hooks/exhaustive-deps
  const run = useCallback((silent = false) => {
    if (!silent) setState((s) => ({ ...s, loading: true, error: null }))
    Promise.resolve(fn())
      .then((data) => setState({ loading: false, data, error: null }))
      .catch((e) =>
        setState((s) => ({
          loading: false,
          data: silent ? s.data : null, // poll lỗi → giữ data cũ
          error: e.message || String(e),
        })),
      )
  }, deps)

  useEffect(() => {
    run(false)
    if (!pollMs) return undefined
    const id = setInterval(() => run(true), pollMs)
    return () => clearInterval(id)
  }, [run, pollMs])

  return { ...state, reload: () => run(false) }
}
