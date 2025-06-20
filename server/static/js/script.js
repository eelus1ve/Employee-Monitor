async function fetchClients() {
    const res = await fetch('/api/clients');
    const data = await res.json();
    const container = document.getElementById("client-table-container");

    if (Object.keys(data).length === 0) {
        container.innerHTML = "<p>Нет подключённых клиентов</p>";
        return;
    }

    let html = `
        <table>
            <tr>
                <th>Имя ПК</th>
                <th>Пользователь</th>
                <th>Публичный IP</th>
                <th>Локальный IP</th>
                <th>Последняя активность</th>
                <th>Действия</th>
            </tr>
    `;

    for (const [cid, info] of Object.entries(data)) {
        html += `
            <tr>
                <td>${info.host}</td>
                <td>${info.user}</td>
                <td>${info.public_ip}</td>
                <td>${info.local_ip}</td>
                <td>${info.last_active}</td>
                <td class="actions">
                    <a class="button screenshot" href="/screenshot/${cid}" target="_blank">Скриншот</a>
                    <button class="button disconnect" onclick="disconnectClient('${cid}')">Отключить</button>
                </td>
            </tr>
        `;
    }

    html += `</table>`;
    container.innerHTML = html;
}

async function disconnectClient(cid) {
    await fetch(`/disconnect/${cid}`);
    fetchClients();
}

setInterval(fetchClients, 5000);
fetchClients();
