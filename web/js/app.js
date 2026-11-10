// Schedule & WorkLog - Frontend Controller
let g_currentProjectId = null;
let g_currentUserId = null;
let g_projects = [];
let g_members = [];
let g_currentFilterDate = 'today';
let g_isPrivate = false;

document.addEventListener('DOMContentLoaded', async () => {
  initEventListeners();
  await loadCurrentUser();
  await loadProjects();
});

function initEventListeners() {
  // Native window controls
  const postWinAction = (act) => {
    if (window.chrome && window.chrome.webview) {
      window.chrome.webview.postMessage(act);
    }
  };

  // 侧边栏折叠/展开逻辑 (Activity Bar 联动)
  const setSidebarCollapsed = (collapsed) => {
    const sidebar = document.getElementById('appSidebar');
    const actBtnProj = document.getElementById('actBtnProjects');
    const toggleSvg = document.getElementById('sidebarToggleSvg');

    if (sidebar) {
      sidebar.classList.toggle('collapsed', collapsed);
    }
    if (actBtnProj) {
      actBtnProj.classList.toggle('active', !collapsed);
    }
    if (toggleSvg) {
      toggleSvg.innerHTML = collapsed 
        ? '<polyline points="9 18 15 12 9 6"></polyline>' 
        : '<polyline points="15 18 9 12 15 6"></polyline>';
    }
    localStorage.setItem('sidebar_collapsed', collapsed ? 'true' : 'false');
  };

  const toggleSidebar = () => {
    const sidebar = document.getElementById('appSidebar');
    const isCollapsed = sidebar ? sidebar.classList.contains('collapsed') : true;
    setSidebarCollapsed(!isCollapsed);
  };

  const actBtnWorkspace = document.getElementById('actBtnWorkspace');
  if (actBtnWorkspace) {
    actBtnWorkspace.addEventListener('click', () => {
      document.querySelectorAll('.activity-icon-btn').forEach(b => b.classList.remove('active'));
      actBtnWorkspace.classList.add('active');
      setSidebarCollapsed(true);
    });
  }

  const actBtnProjects = document.getElementById('actBtnProjects');
  if (actBtnProjects) actBtnProjects.addEventListener('click', toggleSidebar);

  const actBtnToggleSidebar = document.getElementById('actBtnToggleSidebar');
  if (actBtnToggleSidebar) actBtnToggleSidebar.addEventListener('click', toggleSidebar);

  const actBtnSummary = document.getElementById('actBtnSummary');
  if (actBtnSummary) {
    actBtnSummary.addEventListener('click', () => {
      openSummary('personal');
    });
  }

  // 初始化侧边栏状态（默认折叠）
  const savedSidebar = localStorage.getItem('sidebar_collapsed');
  setSidebarCollapsed(savedSidebar !== null ? savedSidebar === 'true' : true);

  const btnMin = document.getElementById('btnWinMin');
  const btnMax = document.getElementById('btnWinMax');
  const btnClose = document.getElementById('btnWinClose');
  if (btnMin) btnMin.addEventListener('click', () => postWinAction('minimize'));
  if (btnMax) btnMax.addEventListener('click', () => postWinAction('maximize'));
  if (btnClose) btnClose.addEventListener('click', () => postWinAction('close'));

  // Window drag via top nav
  const topNav = document.querySelector('.top-nav');
  if (topNav) {
    topNav.addEventListener('mousedown', (e) => {
      if (['BUTTON', 'SELECT', 'INPUT', 'OPTION'].includes(e.target.tagName)) return;
      postWinAction('drag');
    });
    topNav.addEventListener('dblclick', (e) => {
      if (['BUTTON', 'SELECT', 'INPUT', 'OPTION'].includes(e.target.tagName)) return;
      postWinAction('maximize');
    });
  }

  document.getElementById('taskInput').addEventListener('keydown', (e) => {
    if (e.isComposing) return;
    if (e.key === 'Enter') {
      createItem(false); // Enter defaults to "+ 加入待办"
    }
  });

  document.getElementById('btnCreateTodo').addEventListener('click', () => createItem(false));
  const btnMarkDone = document.getElementById('btnMarkDone');
  if (btnMarkDone) {
    btnMarkDone.addEventListener('click', () => createItem(true));
  }

  document.querySelectorAll('.shortcut-tag').forEach(tagEl => {
    tagEl.onclick = () => {
      const input = document.getElementById('taskInput');
      const tag = tagEl.dataset.tag;
      if (!input.value.includes(tag)) {
        input.value = (input.value.trim() + ' ' + tag).trim();
      }
      input.focus();
    };
  });

  // 可见性极简胶囊切换 (公开 / 仅自己可见)
  const visToggleBtn = document.getElementById('visibilityToggleBtn');
  if (visToggleBtn) {
    visToggleBtn.addEventListener('click', () => {
      g_isPrivate = !g_isPrivate;
      visToggleBtn.classList.toggle('is-private', g_isPrivate);
      const icon = document.getElementById('visStatusIcon');
      const label = document.getElementById('visStatusLabel');
      if (g_isPrivate) {
        if (icon) icon.innerText = '🔒';
        if (label) label.innerText = '仅自己可见 (私密)';
        visToggleBtn.title = '当前为私密事项，仅自己可见 (点击切回公开)';
      } else {
        if (icon) icon.innerText = '🌐';
        if (label) label.innerText = '项目公开';
        visToggleBtn.title = '当前为项目公开事项 (点击切换为私密)';
      }
    });
  }

  document.getElementById('userSelector').addEventListener('change', async (e) => {
    const newUserId = e.target.value;
    if (newUserId && newUserId !== g_currentUserId) {
      await fetch('/api/user/switch', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ user_id: newUserId })
      });
      g_currentUserId = newUserId;
      await loadItems();
    }
  });

  document.getElementById('newProjectBtn').addEventListener('click', async () => {
    const name = prompt('请输入新项目名称:');
    if (!name || !name.trim()) return;
    const desc = prompt('请输入项目描述 (可选):') || '';

    const res = await fetch('/api/projects', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ name: name.trim(), description: desc.trim() })
    });
    const data = await res.json();
    if (data.success) {
      await loadProjects();
      selectProject(data.id);
    }
  });

  document.getElementById('addMemberBtn').addEventListener('click', async () => {
    if (!g_currentProjectId) return;
    const name = prompt('请输入新成员姓名:');
    if (!name || !name.trim()) return;

    const res = await fetch('/api/members', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ project_id: g_currentProjectId, name: name.trim(), role: 'member' })
    });
    const data = await res.json();
    if (data.success) {
      await loadCurrentUser();
      await loadMembers();
    }
  });

  document.getElementById('openPersonalSummaryBtn').addEventListener('click', () => openSummary('personal'));
  document.getElementById('openProjectSummaryBtn').addEventListener('click', () => openSummary('project'));

  document.getElementById('closeSummaryModal').addEventListener('click', () => {
    document.getElementById('summaryModal').classList.add('hidden');
  });

  document.getElementById('copySummaryBtn').addEventListener('click', async () => {
    const text = document.getElementById('summaryMarkdownView').value;
    try {
      await navigator.clipboard.writeText(text);
      const tip = document.getElementById('copyTip');
      tip.classList.remove('hidden');
      setTimeout(() => tip.classList.add('hidden'), 2000);
    } catch (e) {
      alert('已选中内容，请按 Ctrl+C 复制');
    }
  });

  // 日期快捷分段 (今天 / 昨天 / 全部)
  document.querySelectorAll('.seg-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      g_currentFilterDate = btn.dataset.date;
      updateDateFilterUI();
      loadItems();
    });
  });

  // 日历胶囊按钮点击唤起系统日期选择
  const calTrigger = document.getElementById('calendarTriggerBtn');
  const nativePicker = document.getElementById('nativeDatePicker');
  if (calTrigger && nativePicker) {
    calTrigger.addEventListener('click', () => {
      if (typeof nativePicker.showPicker === 'function') {
        nativePicker.showPicker();
      } else {
        nativePicker.click();
      }
    });

    nativePicker.addEventListener('change', (e) => {
      const val = e.target.value;
      if (!val) return;
      g_currentFilterDate = val;
      updateDateFilterUI();
      loadItems();
    });
  }

  // 前一天步进
  const prevBtn = document.getElementById('prevDayBtn');
  if (prevBtn) {
    prevBtn.addEventListener('click', () => {
      shiftDate(-1);
    });
  }

  // 后一天步进
  const nextBtn = document.getElementById('nextDayBtn');
  if (nextBtn) {
    nextBtn.addEventListener('click', () => {
      shiftDate(1);
    });
  }
}

function getISODateStr(d) {
  const y = d.getFullYear();
  const m = String(d.getMonth() + 1).padStart(2, '0');
  const day = String(d.getDate()).padStart(2, '0');
  return `${y}-${m}-${day}`;
}

function shiftDate(offsetDays) {
  let baseDate = new Date();
  if (g_currentFilterDate === 'today') {
    baseDate = new Date();
  } else if (g_currentFilterDate === 'yesterday') {
    baseDate = new Date();
    baseDate.setDate(baseDate.getDate() - 1);
  } else if (g_currentFilterDate === 'all') {
    baseDate = new Date();
  } else if (/^\d{4}-\d{2}-\d{2}$/.test(g_currentFilterDate)) {
    const parts = g_currentFilterDate.split('-').map(Number);
    baseDate = new Date(parts[0], parts[1] - 1, parts[2]);
  }

  baseDate.setDate(baseDate.getDate() + offsetDays);
  const targetStr = getISODateStr(baseDate);
  const todayStr = getISODateStr(new Date());
  let yesterday = new Date();
  yesterday.setDate(yesterday.getDate() - 1);
  const yesterdayStr = getISODateStr(yesterday);

  if (targetStr === todayStr) {
    g_currentFilterDate = 'today';
  } else if (targetStr === yesterdayStr) {
    g_currentFilterDate = 'yesterday';
  } else {
    g_currentFilterDate = targetStr;
  }

  updateDateFilterUI();
  loadItems();
}

function updateDateFilterUI() {
  const tabs = document.querySelectorAll('.seg-btn');
  const calBtn = document.getElementById('calendarTriggerBtn');
  const calText = document.getElementById('calendarDateText');
  const nativePicker = document.getElementById('nativeDatePicker');

  const todayStr = getISODateStr(new Date());
  let yesterday = new Date();
  yesterday.setDate(yesterday.getDate() - 1);
  const yesterdayStr = getISODateStr(yesterday);

  if (g_currentFilterDate === todayStr) g_currentFilterDate = 'today';
  if (g_currentFilterDate === yesterdayStr) g_currentFilterDate = 'yesterday';

  tabs.forEach(tab => {
    tab.classList.toggle('active', tab.dataset.date === g_currentFilterDate);
  });

  if (g_currentFilterDate === 'today') {
    if (calBtn) calBtn.classList.remove('active');
    if (calText) calText.innerText = '日历';
    if (nativePicker) nativePicker.value = todayStr;
  } else if (g_currentFilterDate === 'yesterday') {
    if (calBtn) calBtn.classList.remove('active');
    if (calText) calText.innerText = '日历';
    if (nativePicker) nativePicker.value = yesterdayStr;
  } else if (g_currentFilterDate === 'all') {
    if (calBtn) calBtn.classList.remove('active');
    if (calText) calText.innerText = '日历';
    if (nativePicker) nativePicker.value = '';
  } else {
    if (calBtn) calBtn.classList.add('active');
    const parts = g_currentFilterDate.split('-');
    if (calText) calText.innerText = parts.length === 3 ? `${parts[1]}-${parts[2]}` : g_currentFilterDate;
    if (nativePicker) nativePicker.value = g_currentFilterDate;
  }
}

async function loadCurrentUser() {
  const res = await fetch('/api/user/current');
  if (res.ok) {
    const user = await res.json();
    g_currentUserId = user.id;
  }
}

async function loadProjects() {
  const res = await fetch('/api/projects');
  if (!res.ok) return;
  g_projects = await res.json();

  const ul = document.getElementById('projectList');
  ul.innerHTML = '';

  g_projects.forEach(p => {
    const li = document.createElement('li');
    li.className = 'project-item' + (p.id === g_currentProjectId ? ' active' : '');
    li.dataset.id = p.id;
    li.innerHTML = `<span>📁 ${escapeHtml(p.name)}</span><span style="font-size:11px;color:#94a3b8;">${p.member_count}人</span>`;
    li.onclick = () => selectProject(p.id);
    ul.appendChild(li);
  });

  if (!g_currentProjectId && g_projects.length > 0) {
    selectProject(g_projects[0].id);
  }
}

async function selectProject(projId) {
  g_currentProjectId = projId;
  const p = g_projects.find(x => x.id === projId);
  if (p) {
    document.getElementById('currentProjectTitle').innerText = p.name;
    document.getElementById('currentProjectDesc').innerText = p.description || '项目协同工作区';
    const navTabName = document.getElementById('navTabProjectName');
    if (navTabName) navTabName.innerText = p.name;
  }

  document.querySelectorAll('.project-item').forEach(el => {
    el.classList.toggle('active', el.dataset.id === projId);
  });

  await loadMembers();
  await loadItems();
}

async function loadMembers() {
  if (!g_currentProjectId) return;
  const res = await fetch(`/api/members?project_id=${g_currentProjectId}`);
  if (!res.ok) return;
  g_members = await res.json();

  // If current user is not in this project's members, switch to first member
  const currentInProject = g_members.some(m => m.id === g_currentUserId);
  if (!currentInProject && g_members.length > 0) {
    g_currentUserId = g_members[0].id;
    await fetch('/api/user/switch', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ user_id: g_currentUserId })
    });
  }

  const userSelect = document.getElementById('userSelector');
  userSelect.innerHTML = '';
  g_members.forEach(m => {
    const opt = document.createElement('option');
    opt.value = m.id;
    opt.innerText = m.name + (m.role === 'owner' ? ' (Leader)' : '');
    if (m.id === g_currentUserId) opt.selected = true;
    userSelect.appendChild(opt);
  });

  const memberUl = document.getElementById('projectMemberList');
  memberUl.innerHTML = '';
  g_members.forEach(m => {
    const li = document.createElement('li');
    li.className = 'member-item';
    li.innerHTML = `<span>👤 ${escapeHtml(m.name)}</span><span class="member-role">${escapeHtml(m.role)}</span>`;
    memberUl.appendChild(li);
  });
}

async function loadItems() {
  if (!g_currentProjectId || !g_currentUserId) return;
  const res = await fetch(`/api/items?project_id=${g_currentProjectId}&user_id=${g_currentUserId}&date=${encodeURIComponent(g_currentFilterDate)}`);
  if (!res.ok) return;
  const data = await res.json();

  const todoCount = data.todos ? data.todos.length : 0;
  const doneCount = data.dones ? data.dones.length : 0;
  document.getElementById('todoCountPill').innerText = `待办: ${todoCount}`;

  updateDateFilterUI();

  // 动态更新已完成栏标题、统计徽章和空状态提示
  let doneTitle = '✅ 今日已完成';
  let doneEmptyText = '今天还没有记录已完成的工作';
  let donePillText = `今日完成: ${doneCount}`;

  if (g_currentFilterDate === 'today') {
    doneTitle = '✅ 今日已完成';
    doneEmptyText = '今天还没有记录已完成的工作';
    donePillText = `今日完成: ${doneCount}`;
  } else if (g_currentFilterDate === 'yesterday') {
    doneTitle = '📅 昨天已完成';
    doneEmptyText = '昨天没有记录已完成的工作';
    donePillText = `昨天完成: ${doneCount}`;
  } else if (g_currentFilterDate === 'all') {
    doneTitle = '📚 全部历史已完成';
    doneEmptyText = '暂无已完成的工作记录';
    donePillText = `累计完成: ${doneCount}`;
  } else {
    doneTitle = `📅 ${g_currentFilterDate} 已完成`;
    doneEmptyText = `${g_currentFilterDate} 还没有记录已完成的工作`;
    donePillText = `${g_currentFilterDate} 完成: ${doneCount}`;
  }

  const titleEl = document.getElementById('donePanelTitle');
  if (titleEl) titleEl.innerText = doneTitle;
  document.getElementById('doneCountPill').innerText = donePillText;

  // Render Todos
  const todoContainer = document.getElementById('todoListContainer');
  todoContainer.innerHTML = '';
  if (todoCount === 0) {
    todoContainer.innerHTML = '<div class="empty-state">暂无待办事项，输入上方事项添加</div>';
  } else {
    data.todos.forEach(item => {
      const card = document.createElement('div');
      card.className = 'item-card';

      const tagHtml = item.tags ? `<span class="badge-tag">#${escapeHtml(item.tags.replace(/,/g, ' #'))}</span>` : '';
      const privHtml = item.visibility === 'private' ? '<span class="badge-priv">🔒私密</span>' : '';

      card.innerHTML = `
        <input type="checkbox" class="item-checkbox" data-id="${item.id}" title="完成打卡">
        <div class="item-content">${escapeHtml(item.content)}</div>
        <div class="item-meta">
          ${tagHtml}
          ${privHtml}
        </div>
        <button class="btn-icon-delete" title="删除待办" data-id="${item.id}">🗑️</button>
      `;

      card.querySelector('.item-checkbox').onchange = () => markItemDone(item.id);
      card.querySelector('.btn-icon-delete').onclick = (e) => {
        e.stopPropagation();
        deleteItem(item.id, item.content);
      };
      todoContainer.appendChild(card);
    });
  }

  // Render Dones
  const doneContainer = document.getElementById('doneListContainer');
  doneContainer.innerHTML = '';
  if (doneCount === 0) {
    doneContainer.innerHTML = `<div class="empty-state">${escapeHtml(doneEmptyText)}</div>`;
  } else {
    const isToday = g_currentFilterDate === 'today';
    data.dones.forEach(item => {
      const card = document.createElement('div');
      card.className = 'item-card';

      const tagHtml = item.tags ? `<span class="badge-tag">#${escapeHtml(item.tags.replace(/,/g, ' #'))}</span>` : '';
      const privHtml = item.visibility === 'private' ? '<span class="badge-priv">🔒私密</span>' : '';
      const timeStr = item.completed_at ? formatTime(item.completed_at, !isToday) : '';

      card.innerHTML = `
        <input type="checkbox" class="item-checkbox item-checkbox-done" checked data-id="${item.id}" title="点击转回未完成 (撤销打卡)">
        <span class="item-time">${timeStr}</span>
        <div class="item-content">${escapeHtml(item.content)}</div>
        <div class="item-meta">
          ${tagHtml}
          ${privHtml}
        </div>
        <button class="btn-icon-undo" title="恢复为待办 (转回未完成)" data-id="${item.id}">↩</button>
        <button class="btn-icon-delete" title="删除记录" data-id="${item.id}">🗑️</button>
      `;
      card.querySelector('.item-checkbox').onchange = () => markItemTodo(item.id);
      card.querySelector('.btn-icon-undo').onclick = (e) => {
        e.stopPropagation();
        markItemTodo(item.id);
      };
      card.querySelector('.btn-icon-delete').onclick = (e) => {
        e.stopPropagation();
        deleteItem(item.id, item.content);
      };
      doneContainer.appendChild(card);
    });
  }
}

async function createItem(isDone) {
  const input = document.getElementById('taskInput');
  const content = input.value.trim();
  if (!content) return;

  const isPrivate = g_isPrivate ? 1 : 0;

  const res = await fetch('/api/items', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      project_id: g_currentProjectId,
      user_id: g_currentUserId,
      content: content,
      is_done: isDone ? 1 : 0,
      is_private: isPrivate
    })
  });

  if (res.ok) {
    input.value = '';
    if (isDone) {
      g_currentFilterDate = 'today';
    }
    await loadItems();
  }
}

async function markItemDone(itemId) {
  const res = await fetch('/api/items/done', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ item_id: itemId })
  });

  if (res.ok) {
    g_currentFilterDate = 'today';
    await loadItems();
  }
}

async function markItemTodo(itemId) {
  const res = await fetch('/api/items/todo', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ item_id: itemId })
  });

  if (res.ok) {
    await loadItems();
  }
}

async function deleteItem(itemId, itemContent) {
  const shortText = itemContent && itemContent.length > 25 ? itemContent.substring(0, 25) + '...' : (itemContent || '该事项');
  if (!confirm(`确定要删除此项工作记录吗？\n\n"${shortText}"`)) {
    return;
  }

  const res = await fetch('/api/items/delete', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ item_id: itemId })
  });

  if (res.ok) {
    await loadItems();
  } else {
    alert('删除失败，请稍后重试');
  }
}

async function openSummary(type) {
  const modal = document.getElementById('summaryModal');
  const title = document.getElementById('summaryModalTitle');
  const textarea = document.getElementById('summaryMarkdownView');

  let id = type === 'project' ? g_currentProjectId : g_currentUserId;
  title.innerText = type === 'project' ? '📊 本项目全员周报 (已过滤私密)' : '👤 我的个人全项目周总结';
  textarea.value = '正在自动聚合生成周报 Markdown...';
  modal.classList.remove('hidden');

  const res = await fetch(`/api/summary?type=${type}&id=${id}`);
  if (res.ok) {
    const data = await res.json();
    textarea.value = data.markdown;
  } else {
    textarea.value = '生成失败，请重试。';
  }
}

function formatTime(tsMs, showDate = false) {
  const d = new Date(tsMs);
  const h = String(d.getHours()).padStart(2, '0');
  const m = String(d.getMinutes()).padStart(2, '0');
  if (showDate) {
    const mon = String(d.getMonth() + 1).padStart(2, '0');
    const day = String(d.getDate()).padStart(2, '0');
    return `${mon}-${day} ${h}:${m}`;
  }
  return `${h}:${m}`;
}

function escapeHtml(str) {
  if (!str) return '';
  return str.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}
