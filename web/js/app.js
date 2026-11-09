// Schedule & WorkLog - Frontend Controller
let g_currentProjectId = null;
let g_currentUserId = null;
let g_projects = [];
let g_members = [];

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
      createItem(true); // Enter defaults to "⚡ 记为已完成"
    }
  });

  document.getElementById('btnMarkDone').addEventListener('click', () => createItem(true));
  document.getElementById('btnCreateTodo').addEventListener('click', () => createItem(false));

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
  const res = await fetch(`/api/items?project_id=${g_currentProjectId}&user_id=${g_currentUserId}`);
  if (!res.ok) return;
  const data = await res.json();

  const todoCount = data.todos ? data.todos.length : 0;
  const doneCount = data.dones ? data.dones.length : 0;
  document.getElementById('todoCountPill').innerText = `待办: ${todoCount}`;
  document.getElementById('doneCountPill').innerText = `今日完成: ${doneCount}`;

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
        <input type="checkbox" class="item-checkbox" data-id="${item.id}">
        <div class="item-content">${escapeHtml(item.content)}</div>
        <div class="item-meta">
          ${tagHtml}
          ${privHtml}
        </div>
      `;

      card.querySelector('.item-checkbox').onchange = () => markItemDone(item.id);
      todoContainer.appendChild(card);
    });
  }

  // Render Dones
  const doneContainer = document.getElementById('doneListContainer');
  doneContainer.innerHTML = '';
  if (doneCount === 0) {
    doneContainer.innerHTML = '<div class="empty-state">今天还没有记录已完成的工作</div>';
  } else {
    data.dones.forEach(item => {
      const card = document.createElement('div');
      card.className = 'item-card';

      const tagHtml = item.tags ? `<span class="badge-tag">#${escapeHtml(item.tags.replace(/,/g, ' #'))}</span>` : '';
      const privHtml = item.visibility === 'private' ? '<span class="badge-priv">🔒私密</span>' : '';
      const timeStr = item.completed_at ? formatTime(item.completed_at) : '';

      card.innerHTML = `
        <span style="color:#10b981;font-weight:bold;">✓</span>
        <span class="item-time">${timeStr}</span>
        <div class="item-content">${escapeHtml(item.content)}</div>
        <div class="item-meta">
          ${tagHtml}
          ${privHtml}
        </div>
      `;
      doneContainer.appendChild(card);
    });
  }
}

async function createItem(isDone) {
  const input = document.getElementById('taskInput');
  const content = input.value.trim();
  if (!content) return;

  const vis = document.getElementById('visibilitySelect').value;
  const isPrivate = vis === 'private' ? 1 : 0;

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
    await loadItems();
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

function formatTime(tsMs) {
  const d = new Date(tsMs);
  const h = String(d.getHours()).padStart(2, '0');
  const m = String(d.getMinutes()).padStart(2, '0');
  return `${h}:${m}`;
}

function escapeHtml(str) {
  if (!str) return '';
  return str.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}
